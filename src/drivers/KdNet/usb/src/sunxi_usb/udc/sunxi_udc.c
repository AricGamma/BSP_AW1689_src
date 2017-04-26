/** @file
*
*  Copyright (c) 2007-2016, Allwinner Technology Co., Ltd. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**/

#undef DEBUG
#include <malloc.h>

#include  "sunxi_udc_config.h"
#include  "sunxi_udc_board.h"
#include  "sunxi_udc_dma.h"

/***********************************************************/

#define DRIVER_DESC "SoftWinner USB Device Controller"
#define DRIVER_VERSION "20080411"
#define DRIVER_AUTHOR	"SoftWinner USB Developer"

struct sunxi_udc *the_controller = NULL;
typedef struct __usbc_otg{
	uint  port_num;
	ulong base_addr;        /* usb base address */

	uint used;
	uint no;
}__usbc_otg_t;

static const char gadget_name[] = "sunxi_usb_udc";
static const char driver_desc[] = DRIVER_DESC;

#define	DMA_ADDR_INVALID	(~(dma_addr_t)0)

static u8 crq_bRequest = 0;
static u8 crq_wIndex = 0;
static u8 crq_dir = 0;
static const unsigned char TestPkt[54] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xEE, 0xEE, 0xEE,
	0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xBF, 0xDF,
	0xEF, 0xF7, 0xFB, 0xFD, 0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7,
	0xFB, 0xFD, 0x7E, 0x00
};

static u32 usbd_port_no = 0;
static sunxi_udc_io_t g_sunxi_udc_io;
static u32 is_controller_alive = 0;
static __u32 dma_working = 0;
static u32 usb_connect = 0;

static int g_write_debug = 0;
static int g_read_debug = 0;
static int g_queue_debug = 0;
static int g_dma_debug = 0;
static int g_irq_debug = 0;

/*
  Local declarations.
*/
static void sunxi_udc_stop_dma_work(struct sunxi_udc *dev, u32 unlock);
static void clear_all_irq(void);
static void cfg_udc_command(enum sunxi_udc_cmd_e cmd);

static __u32 is_peripheral_active(void)
{
	return is_controller_alive;
}

/* DMA transfer conditions:
 * 1. the driver support dma transfer
 * 2. not EP0
 * 3. more than one packet
 */
#define  big_req(req, ep) ((req->length != req->actual) \
			   ? ((req->length - req->actual) > ep->maxpacket) \
			   : (req->length > ep->maxpacket))
#define  is_sunxi_udc_dma_capable(req, ep) (is_udc_support_dma() \
						&& big_req(req, ep) \
						&& req->dma_flag \
						&& ep->num)

#define is_buffer_mapped(req, ep) (is_sunxi_udc_dma_capable(req, ep) \
				   && (req->map_state != UN_MAPPED))

/*
 * udc_disable - disable USB device controller
 */
static void sunxi_udc_disable(struct sunxi_udc *dev)
{
	DMSG_DBG_UDC("sunxi_udc_disable\n");

	/* Disable all interrupts */
	USBC_INT_DisableUsbMiscAll(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_INT_DisableEpAll(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_RX);
	USBC_INT_DisableEpAll(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_TX);

	/* Clear the interrupt registers */
	clear_all_irq();
	cfg_udc_command(SW_UDC_P_DISABLE);

	/* Set speed to unknown */
	dev->speed = UsbBusSpeedUnknown;
	return;
}

/*
 *	udc_reinit - initialize software state
 */
static void sunxi_udc_reinit(struct sunxi_udc *dev)
{
	u32 i = 0;

	/* device/ep0 records init */
	dev->ep0state = EP0_IDLE;
	dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;

	for (i = 0; i < SW_UDC_ENDPOINTS; i++) {
		struct sunxi_udc_ep *ep = &dev->ep[i];

		ep->dev	 = dev;
		ep->desc = NULL;
		ep->halted  = 0;
		INIT_LIST_HEAD (&ep->queue);
	}

	for (i = 0; i < DMA_BUFFERS; i++) {
		dev->fifo_req[i].dma = DMA_ADDR_INVALID;
		dev->fifo_req[i].map_state = UN_MAPPED;
		dev->fifo_req[i].req_num = i;
	}

	return;
}

/* until it's enabled, this UDC should be completely invisible
 * to any USB host.
 */
static void sunxi_udc_enable(struct sunxi_udc *dev)
{
	UNREFERENCED_PARAMETER(dev);

	DMSG_DBG_UDC("sunxi_udc_enable called\n");

	/* dev->gadget.speed = USB_SPEED_UNKNOWN; */
	dev->speed = UsbBusSpeedUnknown;

#ifdef	CONFIG_USB_GADGET_DUALSPEED
	DMSG_INFO_UDC("CONFIG_USB_GADGET_DUALSPEED: USBC_TS_MODE_HS\n");

	USBC_Dev_ConfigTransferMode(g_sunxi_udc_io.usb_bsp_hdle, USBC_TS_TYPE_BULK, USBC_TS_MODE_HS);
#else
	DMSG_INFO_UDC("CONFIG_USB_GADGET_DUALSPEED: USBC_TS_MODE_FS\n");

	USBC_Dev_ConfigTransferMode(g_sunxi_udc_io.usb_bsp_hdle, USBC_TS_TYPE_BULK, USBC_TS_MODE_FS);
#endif
	/* Enable reset and suspend interrupt interrupts */
	USBC_INT_EnableUsbMiscUint(g_sunxi_udc_io.usb_bsp_hdle, USBC_BP_INTUSB_SUSPEND);
	USBC_INT_EnableUsbMiscUint(g_sunxi_udc_io.usb_bsp_hdle, USBC_BP_INTUSB_RESUME);
	USBC_INT_EnableUsbMiscUint(g_sunxi_udc_io.usb_bsp_hdle, USBC_BP_INTUSB_RESET);
	USBC_INT_EnableUsbMiscUint(g_sunxi_udc_io.usb_bsp_hdle, USBC_BP_INTUSB_DISCONNECT);

	/* Enable ep0 interrupt */
	USBC_INT_EnableEp(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_TX, 0);

	cfg_udc_command(SW_UDC_P_ENABLE);

	return ;
}

/*
  Start the peripheral controller.
*/
int sunxi_start_udc(PVOID Miniport, PDEBUG_DEVICE_DESCRIPTOR Device,
			 PVOID DmaBuffer)
{
	UNREFERENCED_PARAMETER(Miniport);
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(DmaBuffer);

	struct sunxi_udc *udc = the_controller;
	int retval = 0;
	int i = 0;

	usbd_port_no = 0;
	usb_connect = 0;
	crq_bRequest = 0;
	crq_dir = 0;
	is_controller_alive = 1;

	memset(&g_sunxi_udc_io, 0, sizeof(sunxi_udc_io_t));

	retval = sunxi_udc_io_init(Miniport,usbd_port_no, &g_sunxi_udc_io);
	if (retval != 0) {
		DMSG_PANIC("ERR: sunxi_udc_io_init failed\n");
		return -1;
	}

	/* Sanity checks */
	if (!udc) {
		DMSG_PANIC("ERR: sunxi_start_udc, udc is null\n");
		return -ENODEV;
	}

	sunxi_udc_disable(udc);

	udc->sunxi_udc_io = &g_sunxi_udc_io;
	udc->usbc_no = usbd_port_no;
	strcpy((char *)udc->driver_name, gadget_name);

	if (is_udc_support_dma()) {
		retval = sunxi_udc_dma_probe(udc);
		if (retval != 0) {
			DMSG_PANIC("ERR: sunxi_udc_dma_probe failed\n");
			retval = -EBUSY;
			goto err;
		}
	}

	/* Hook the driver */

	/* Enable udc */
	sunxi_udc_enable(udc);

	for (i = 0; i < DMA_BUFFERS; i++ ) {
		INIT_LIST_HEAD(&udc->fifo_req[i].queue);
	}

	return 0;

err:
	if (is_udc_support_dma())
		sunxi_udc_dma_remove(udc);

	sunxi_udc_io_exit(usbd_port_no, &g_sunxi_udc_io);

	return retval;
}

/*
 * Stop the peripheral controller.
 */
int sunxi_stop_udc(PVOID Miniport)
{
	UNREFERENCED_PARAMETER(Miniport);

	struct sunxi_udc *udc = the_controller;

	if (!udc) {
		DMSG_PANIC("ERR: sunxi_stop_udc, udc is null\n");
		return -ENODEV;
	}

	DMSG_INFO_UDC("[%s]: sunxi_stop_udc()\n", gadget_name);

	if (is_udc_support_dma()) {
		sunxi_udc_stop_dma_work(udc, 0);
		sunxi_udc_dma_remove(udc);
	}

	/* unbind gadget driver */

	/* Disable udc */
	sunxi_udc_disable(udc);

	sunxi_udc_io_exit(usbd_port_no, &g_sunxi_udc_io);

	memset(&g_sunxi_udc_io, 0, sizeof(sunxi_udc_io_t));

	usbd_port_no = 0;
	usb_connect = 0;
	crq_bRequest = 0;
	crq_dir = 0;
	is_controller_alive = 0;

	return 0;
}

/* Maps the buffer to dma  */
static inline void sunxi_udc_map_dma_buffer(
						PHYSICAL_ADDRESS pa,
						struct sunxi_udc_request *req,
						struct sunxi_udc_ep *ep)
{
	if (!is_sunxi_udc_dma_capable(req, ep)) {
		DMSG_PANIC("err: need not to dma map\n");
		return;
	}

	req->map_state = UN_MAPPED;

	if (req->dma == DMA_ADDR_INVALID) {
		req->dma = pa.u.LowPart;
		req->map_state = SW_UDC_USB_MAPPED;
	} else {
		req->map_state = PRE_MAPPED;
	}

	return;
}

/* Unmap the buffer from dma and maps it back to cpu */
static inline void sunxi_udc_unmap_dma_buffer(
						struct sunxi_udc_request *req,
						struct sunxi_udc *udc,
						struct sunxi_udc_ep *ep)
{
	UNREFERENCED_PARAMETER(udc);

	if (!is_buffer_mapped(req, ep)) {
		//DMSG_PANIC("err: need not to dma ummap\n");
		return;
	}

	if (req->dma == DMA_ADDR_INVALID) {
		DMSG_PANIC("not unmapping a never mapped buffer\n");
		return;
	}

	if (req->map_state == SW_UDC_USB_MAPPED) {
		dma_unmap_single((void *)req->dma, req->length,
				 (is_tx_ep(ep) ? DMA_TO_DEVICE
				  : DMA_FROM_DEVICE));

		req->dma = DMA_ADDR_INVALID;
	} else { /* PRE_MAPPED */
		dma_sync_single_for_cpu(req->dma, req->length,
					(is_tx_ep(ep) ? DMA_TO_DEVICE
					: DMA_FROM_DEVICE));
	}

	req->map_state = UN_MAPPED;

	return;
}

/* disable usb module irq */
static void disable_irq_udc(struct sunxi_udc *dev)
{
	UNREFERENCED_PARAMETER(dev);
	//disable_irq(dev->irq_no);
}

/* enable usb module irq */
static void enable_irq_udc(struct sunxi_udc *dev)
{
	UNREFERENCED_PARAMETER(dev);
	//enable_irq(dev->irq_no);
}

static void sunxi_udc_done(struct sunxi_udc_ep *ep,
				  struct sunxi_udc_request *req,
				  int status)
{
	UNREFERENCED_PARAMETER(status);

	unsigned halted = ep->halted;

	if(g_queue_debug){
		DMSG_INFO_UDC("d: (%d, %p, %d, %d)\n\n\n", ep->num, req,
			      req->length, req->actual);
	}

	ep->halted = 1;
	if (is_sunxi_udc_dma_capable(req, ep)) {
		sunxi_udc_unmap_dma_buffer(req, ep->dev, ep);
	}
	ep->halted = halted;

	if(g_queue_debug){
		DMSG_INFO_UDC("%s:%d: %s, %p\n", __func__, __LINE__, ep->num, req);
	}
}

/*
 *	nuke - dequeue ALL requests
 */
static void sunxi_udc_nuke(struct sunxi_udc *udc, struct sunxi_udc_ep *ep,
				  int status)
{
	UNREFERENCED_PARAMETER(udc);

	/* Sanity check */
	if (&ep->queue == NULL)
		return;

	while (!list_empty (&ep->queue)) {
		struct sunxi_udc_request *req;
		{
			const struct list_head *__mptr = (ep->queue.next);
			req = (struct sunxi_udc_request *)( (char *)__mptr
				- offsetof(struct sunxi_udc_request,queue) );
		}
		DMSG_INFO_UDC("nuke: ep num is %d\n", ep->num);
		sunxi_udc_done(ep, req, status);
		list_del_init(&req->queue);
	}	
}

static inline int sunxi_udc_fifo_count_out(__hdle usb_bsp_hdle,
						    __u8 ep_index)
{
	UNREFERENCED_PARAMETER(usb_bsp_hdle);

	if (ep_index) {
		return USBC_ReadLenFromFifo(g_sunxi_udc_io.usb_bsp_hdle,
					    USBC_EP_TYPE_RX);
	} else {
		return USBC_ReadLenFromFifo(g_sunxi_udc_io.usb_bsp_hdle,
					    USBC_EP_TYPE_EP0);
	}
}

static inline int sunxi_udc_write_packet(int fifo,
						struct sunxi_udc_request *req,
						unsigned max)
{
	unsigned len = min(req->length - req->actual, max);
	u8 *buf = (u8 *)req->buf + req->actual;

	prefetch(buf);

	DMSG_DBG_UDC("W: req.actual(%d), req.length(%d), len(%d), total(%d)\n",
		req->actual, req->length, len, req->actual + len);

	req->actual += len;

	KeStallExecutionProcessor(5); /* equal to udelay(5) */
	USBC_WritePacket(g_sunxi_udc_io.usb_bsp_hdle, fifo, len, buf);

	return len;
}

static int pio_write_fifo(struct sunxi_udc_ep *ep, struct sunxi_udc_request *req)
{
	unsigned	count		= 0;
	int		is_last		= 0;
	u8		idx		= 0;
	int		fifo_reg	= 0;
	__s32		ret		= 0;
	u8		old_ep_index	= 0;

	idx = ep->bEndpointAddress & 0x7F;

	/* write data */

	/* select ep */
	old_ep_index = (u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8) idx);

	/* select fifo */
	fifo_reg = USBC_SelectFIFO(g_sunxi_udc_io.usb_bsp_hdle, idx);

	count = sunxi_udc_write_packet(fifo_reg, req, ep->maxpacket);

	/* check if the last packet */

	/* last packet is often short (sometimes a zlp) */
	if (count != ep->maxpacket)
		is_last = 1;
	else if (req->length != req->actual || req->zero)
		is_last = 0;
	else
		is_last = 2;

	if (g_write_debug) {
		DMSG_INFO_UDC("pw: (0x%p, %d, %d)\n", req, req->length,
			req->actual);
	}

	if (idx) { /* ep1~4 */
		ret = USBC_Dev_WriteDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_EP_TYPE_TX, is_last);
		if (ret != 0) {
			DMSG_PANIC("ERR: USBC_Dev_WriteDataStatus, failed\n");
		}
	} else {  /* ep0 */
		ret = USBC_Dev_WriteDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_EP_TYPE_EP0, is_last);
		if (ret != 0) {
			DMSG_PANIC("ERR: USBC_Dev_WriteDataStatus, failed\n");
		}
	}

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	if (is_last) {
		if (!idx) {  /* ep0 */
			ep->dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		}

		sunxi_udc_done(ep,req, 0);
		is_last = 1;
	}

	return is_last;
}

static int dma_write_fifo(struct sunxi_udc_ep *ep,
		struct sunxi_udc_request *req)
{
	u32   	left_len	= 0;
	u8	idx		= 0;
	int	fifo_reg	= 0;
	u8	old_ep_index	= 0;

	idx = ep->bEndpointAddress & 0x7F;

	/* select ep */
	old_ep_index = USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, idx);

	/* select fifo */
	fifo_reg = USBC_SelectFIFO(g_sunxi_udc_io.usb_bsp_hdle, idx);

	/* auto_set, tx_mode, dma_tx_en, mode1 */
	USBC_Dev_ConfigEpDma(ep->dev->sunxi_udc_io->usb_bsp_hdle,
			USBC_EP_TYPE_TX);

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);


	/* cut fragment part(??) */
	left_len = req->length - req->actual;
	left_len = left_len - (left_len % ep->maxpacket);

	ep->dma_working	= 1;
	dma_working = 1;
	ep->dma_transfer_len = left_len;

	if (g_dma_debug) {
		DMSG_INFO_UDC("dw: (0x%p, %d, %d)\n", &(req), req->length,
			      req->actual);
	}

	sunxi_udc_dma_set_config(ep, req, (__u32)req->dma, left_len);
	sunxi_udc_dma_start(ep, fifo_reg, (__u32)req->dma, left_len);

	return 0;
}

static void __usb_writecomplete(__hdle hUSB, u32 ep_type, u32 complete)
{
	USBC_Dev_WriteDataStatus(hUSB, ep_type, complete);

	/* wait for tx packet sent out */
	while(USBC_Dev_IsWriteDataReady(hUSB, ep_type));

	return;
}

static int __usb_write_fifo(struct sunxi_udc_ep *ep,
			struct sunxi_udc_request *req)
{
	u8 old_ep_idx = 0;
	u32 fifo = 0;
	u32 transfered = 0;
	u32 left = 0;
	u32 this_len;
	uchar *buffer = NULL;
	unsigned int buffer_size = 0;
	u8 idx = 0;
	int is_last = 0;

	idx = ep->bEndpointAddress & 0x7F;
	buffer = req->buf;
	buffer_size = req->length;

	/* Save index */
	old_ep_idx = USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8)idx);

	left = buffer_size;
	fifo = USBC_SelectFIFO(g_sunxi_udc_io.usb_bsp_hdle, (__u8)idx);
	while(left)
	{
		this_len = min(512, left);
		this_len = USBC_WritePacket(g_sunxi_udc_io.usb_bsp_hdle, fifo,
			this_len, buffer + transfered);

		transfered += this_len;
		left -= this_len;
		req->actual = transfered;
		if(left == 0) {
			is_last = 1;
		}

		__usb_writecomplete(g_sunxi_udc_io.usb_bsp_hdle,
				USBC_EP_TYPE_TX, 1);
	}

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_idx);

	return is_last;
}



/* return: 0 = still running, 1 = completed, negative = errno */
static int sunxi_udc_write_fifo(struct sunxi_udc_ep *ep,
			struct sunxi_udc_request *req)
{
	u32 idx = 0;
	int is_in = 0;

	idx = ep->bEndpointAddress & 0x7F;
	is_in = ep->bEndpointAddress & USB_DIR_IN;

	if (ep->dma_working) {
		if (g_dma_debug) {
			DMSG_DBG_UDC("dma is busy");
		}

		return 0;
	}

	if (is_sunxi_udc_dma_capable(req, ep))
		return dma_write_fifo(ep, req);
	else {
		if (idx != 0 && is_in)
			return __usb_write_fifo(ep, req);
		else
			return pio_write_fifo(ep, req);

	}
}

static inline int sunxi_udc_read_packet(int fifo, u8 *buf,
		struct sunxi_udc_request *req, unsigned avail)
{
	unsigned len = 0;

	len = min(req->length - req->actual, avail);
	req->actual += len;

	DMSG_DBG_UDC("R: req.actual(%d), req.length(%d), len(%d), total(%d)\n",
		req->actual, req->length, len, req->actual + len);

	USBC_ReadPacket(g_sunxi_udc_io.usb_bsp_hdle, fifo, len, buf);

	return len;
}

/* return: 0 = still running, 1 = completed, negative = errno */
static int pio_read_fifo(struct sunxi_udc_ep *ep,
			     struct sunxi_udc_request *req)
{
	u8		*buf 		= NULL;
	unsigned	bufferspace	= 0;
	int		is_last		= 1;
	unsigned	avail 		= 0;
	int		fifo_count 	= 0;
	u32		idx 		= 0;
	int		fifo_reg 	= 0;
	__s32 		ret 		= 0;
	u8		old_ep_index 	= 0;

	idx = ep->bEndpointAddress & 0x7F;

	/* select fifo */
	fifo_reg = USBC_SelectFIFO(g_sunxi_udc_io.usb_bsp_hdle, idx);

	if (!req->length) {
		DMSG_PANIC("ERR: req->length == 0\n");
		return 1;
	}

	buf = (u8 *)req->buf + req->actual;
	bufferspace = req->length - req->actual;
	if (!bufferspace) {
		DMSG_PANIC("ERR: buffer full!\n");
		return -1;
	}

	/* select ep */
	old_ep_index = (u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8) idx);

	fifo_count = sunxi_udc_fifo_count_out(g_sunxi_udc_io.usb_bsp_hdle,
					      (__u8) idx);
	if (fifo_count > (int) ep->maxpacket) {
		avail = ep->maxpacket;
	} else {
		avail = fifo_count;
	}

	fifo_count = sunxi_udc_read_packet(fifo_reg, buf, req, avail);

	/* checking this with ep0 is not accurate as we already
	 * read a control request */
	if (idx != 0 && fifo_count < (int) ep->maxpacket) {
		is_last = 1;
		/* overflowed this request?  flush extra data */
		if (fifo_count != (int) avail) {
			//req->req.status = -EOVERFLOW;
		}
	} else {
		is_last = (req->length <= req->actual) ? 1 : 0;
	}

	if (g_read_debug) {
		DMSG_INFO_UDC("pr: (0x%p, %d, %d)\n", req, req->length,
			      req->actual);
	}

	if (idx) {
		ret = USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
					      USBC_EP_TYPE_RX, is_last);
		if (ret != 0) {
			DMSG_PANIC("ERR: pio_read_fifo: USBC_Dev_WriteDataStatus, failed\n");
		}
	} else {
		ret = USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
					      USBC_EP_TYPE_EP0, is_last);
		if (ret != 0) {
			DMSG_PANIC("ERR: pio_read_fifo: USBC_Dev_WriteDataStatus, failed\n");
		}
	}

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	if (is_last) {
		if (!idx) {
			ep->dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		}

		sunxi_udc_done(ep, req, 0);
		is_last = 1;
	}

	return is_last;
}

static int __pio_read_fifo(struct sunxi_udc_ep *ep,
				struct sunxi_udc_request *req)
{
	u32 idx = 0;
	u8 old_ep_index = 0;
	u8 is_ep_pending_cleared = 1;
	int ret = 0;

	idx = ep->bEndpointAddress & 0x7F;
	/* select ep */
	old_ep_index = (u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8) idx);

	while (ret == 0) {
		if (is_ep_pending_cleared == 0)
			USBC_INT_ClearEpPending(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_RX, (__u8) idx);
		is_ep_pending_cleared = 0;

		ret = pio_read_fifo(ep, req);
		/* wait for read packet */
		if (ret == 0)
			while (!USBC_Dev_IsReadDataReady(g_sunxi_udc_io.usb_bsp_hdle,
						 USBC_EP_TYPE_RX));
	}

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	return ret;
}

static int dma_read_fifo(struct sunxi_udc_ep *ep, struct sunxi_udc_request *req)
{
	u32   	left_len 	= 0;
	u8	idx		= 0;
	int	fifo_reg	= 0;
	u8	old_ep_index 	= 0;

	idx = ep->bEndpointAddress & 0x7F;

	/* select ep */
	old_ep_index = USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, idx);

	/* select fifo */
	fifo_reg = USBC_SelectFIFO(g_sunxi_udc_io.usb_bsp_hdle, idx);

	/* auto_set, tx_mode, dma_tx_en, mode1 */
	USBC_Dev_ConfigEpDma(ep->dev->sunxi_udc_io->usb_bsp_hdle,
			USBC_EP_TYPE_RX);

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	/* cut fragment packet part */
	left_len = req->length - req->actual;
	left_len = left_len - (left_len % ep->maxpacket);

	if (g_dma_debug) {
		DMSG_INFO_UDC("dr: (0x%p, %d, %d),%x\n", req, req->length,
			      req->actual,req->dma);
	}

	ep->dma_working	= 1;
	dma_working = 1;
	ep->dma_transfer_len = left_len;

	sunxi_udc_dma_set_config(ep, req, (__u32)req->dma, left_len);
	sunxi_udc_dma_start(ep, fifo_reg, (__u32)req->dma, left_len);

	return 0;
}

/* return: 0 = still running, 1 = completed, negative = errno */
static int sunxi_udc_read_fifo(struct sunxi_udc_ep *ep,
			struct sunxi_udc_request *req)
{
	u32 idx = 0;
	int is_in = 0;

	idx = ep->bEndpointAddress & 0x7F;
	is_in = ep->bEndpointAddress & USB_DIR_IN;

	if (ep->dma_working) {
		return 0;
	}

	if (is_sunxi_udc_dma_capable(req, ep)) {
		return dma_read_fifo(ep, req);
	} else {
		if (idx != 0 && !is_in)
			return __pio_read_fifo(ep, req);
		else
			return pio_read_fifo(ep, req);
	}
}

static int sunxi_udc_read_fifo_crq(PUSB_DEFAULT_PIPE_SETUP_PACKET crq)
{
	u32 fifo_count  = 0;
	u32 i		= 0;
	u8  *pOut	= (u8 *) crq;
	u32 fifo	= 0;

	fifo = USBC_SelectFIFO(g_sunxi_udc_io.usb_bsp_hdle, 0);
	fifo_count = USBC_ReadLenFromFifo(g_sunxi_udc_io.usb_bsp_hdle,
					USBC_EP_TYPE_EP0);

	if (fifo_count != 8) {
		i = 0;

		while(i < 16 && (fifo_count != 8) ) {
			fifo_count = USBC_ReadLenFromFifo(
					g_sunxi_udc_io.usb_bsp_hdle,
					USBC_EP_TYPE_EP0);
			i++;
		}

		if (i >= 16) {
			DMSG_PANIC("ERR: get ep0 fifo len failed\n");
		}
	}

	return USBC_ReadPacket(g_sunxi_udc_io.usb_bsp_hdle, fifo, fifo_count,
			pOut);
}

static void sunxi_vendor_or_class_req(struct sunxi_udc *dev,
				    PUSB_DEFAULT_PIPE_SETUP_PACKET crq)
{
	int is_dev = 0, is_intf = 0;

	is_dev = ((crq->bmRequestType.B & USB_RECIP_MASK)
		  == USB_RECIP_DEVICE);
	is_intf = ((crq->bmRequestType.B & USB_RECIP_MASK)
		   == USB_RECIP_INTERFACE);

	switch (crq->bRequest) {
	case VENDOR_REQ_WINUSB:
		DMSG_DBG_UDC("VENDOR_REQ_WINUSB ... \n");

		if (is_dev || is_intf) {
			//rx receive over, dataend, tx_pakect ready
			USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_EP0, 0);

			dev->ep0state = EP0_END_XFER;
			dev->ep0_data_trans = EP0_DATA_TRANS_BUSY;
			crq_bRequest = VENDOR_REQ_WINUSB;
			crq_dir = crq->bmRequestType.B & USB_DIR_IN;

			return;
		}
		break;
	case CLASS_REQ_SET_CONTROL_LINE_STATE:
		DMSG_DBG_UDC("CLASS_REQ_SET_CONTROL_LINE_STATE ... \n");

		if (is_intf) {
			dev->ep0state = EP0_STATUS;
			dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
			crq_bRequest = CLASS_REQ_SET_CONTROL_LINE_STATE;
			crq_dir = crq->bmRequestType.B & USB_DIR_IN;

			return;
		}
		break;
	default:
		/* only receive setup_data packet, cannot set DataEnd */
		USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
					USBC_EP_TYPE_EP0, 0);
		break;
	}

}

static void sunxi_udc_handle_ep0_idle(struct sunxi_udc *dev,
					struct sunxi_udc_ep *ep,
					PUSB_DEFAULT_PIPE_SETUP_PACKET crq,
					u32 ep0csr,
					PUSBFNMP_MESSAGE message,
					PUSBFNMP_MESSAGE_PAYLOAD payload)
{
	UNREFERENCED_PARAMETER(ep0csr);

	int len = 0, tmp = 0, is_vendor_req = 0, is_class_req = 0;

	/* start control request? */
	if (!USBC_Dev_IsReadDataReady(g_sunxi_udc_io.usb_bsp_hdle,
	    USBC_EP_TYPE_EP0)) {
		DMSG_WRN("ERR: data is ready, can not read data.\n");
		return;
	}

	sunxi_udc_nuke(dev, ep, -EPROTO);

	len = sunxi_udc_read_fifo_crq(crq);
	if (len != sizeof(*crq)) {
		DMSG_PANIC("setup begin: fifo READ ERROR"
			" wanted %d bytes got %d. Stalling out...\n",
			sizeof(*crq), len);

		USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_EP_TYPE_EP0, 0);
		USBC_Dev_EpSendStall(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_EP_TYPE_EP0);

		return;
	}

	DMSG_DBG_UDC("ep0: bRequest = %d bmRequestType %d wLength = %d\n",
		crq->bRequest, crq->bmRequestType, crq->wLength);

	/* cope with automagic for some standard requests. */
	dev->req_std = ((crq->bmRequestType.B & USB_TYPE_MASK) == USB_TYPE_STANDARD);
	dev->req_config = 0;
	dev->req_pending = 1;
	is_vendor_req = ((crq->bmRequestType.B & USB_TYPE_MASK) == USB_TYPE_VENDOR);
	is_class_req = ((crq->bmRequestType.B & USB_TYPE_MASK) == USB_TYPE_CLASS);

	RtlCopyMemory(&payload->udr, crq, sizeof(payload->udr));
	*message = UsbMsgSetupPacket;

	if (dev->req_std) {   //standard request
		switch (crq->bRequest) {
		case USB_REQ_SET_ADDRESS:
			DMSG_DBG_UDC("USB_REQ_SET_ADDRESS ... \n");

			if (crq->bmRequestType.B == USB_RECIP_DEVICE) {
				tmp = crq->wValue.W & 0x7F;
				dev->address = tmp;

				//rx receive over, dataend, tx_pakect ready
				USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
							USBC_EP_TYPE_EP0, 1);

				USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, 0);

				USBC_Dev_Ctrl_ClearSetupEnd(g_sunxi_udc_io.usb_bsp_hdle);
				KeStallExecutionProcessor(1000); /* 1ms */
				USBC_Dev_SetAddress(g_sunxi_udc_io.usb_bsp_hdle,
						(u8) dev->address);

				DMSG_INFO_UDC("Set address %d,%d\n", dev->address,
					USBC_Readw((USHORT *) (dev->sunxi_udc_io->usb_pbase)
					+ USBC_REG_o_FADDR));

				dev->ep0state = EP0_STATUS;
				dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
				crq_bRequest = USB_REQ_SET_ADDRESS;
				crq_dir = crq->bmRequestType.B & USB_DIR_IN;

				return;
			}
			break;
		case USB_REQ_SET_CONFIGURATION:
			DMSG_DBG_UDC("USB_REQ_SET_CONFIGURATION ... \n");

			if (crq->bmRequestType.B == USB_RECIP_DEVICE) {
				dev->ep0state = EP0_STATUS;
				dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
				crq_bRequest = USB_REQ_SET_CONFIGURATION;
				crq_dir = crq->bmRequestType.B & USB_DIR_IN;

				return;
			}
			break;
		case USB_REQ_GET_DESCRIPTOR:
			DMSG_DBG_UDC("USB_REQ_GET_DESCRIPTOR ... \n");

			if ((crq->bmRequestType.B & USB_RECIP_MASK)
			    == USB_RECIP_DEVICE) {
				//rx receive over, dataend, tx_pakect ready
				USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
					USBC_EP_TYPE_EP0, 0);

				dev->ep0state = EP0_END_XFER;
				dev->ep0_data_trans = EP0_DATA_TRANS_BUSY;
				crq_bRequest = USB_REQ_GET_DESCRIPTOR;
				crq_dir = crq->bmRequestType.B & USB_DIR_IN;

				return;
			}
			break;
		case USB_REQ_GET_STATUS:
			DMSG_DBG_UDC("USB_REQ_GET_STATUS ... \n");

			USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
					USBC_EP_TYPE_EP0, 0);
			dev->ep0state = EP0_END_XFER;
			dev->ep0_data_trans = EP0_DATA_TRANS_BUSY;
			crq_bRequest = USB_REQ_GET_STATUS;
			crq_dir = crq->bmRequestType.B & USB_DIR_IN;
			break;
		default:
			/* only receive setup_data packet, cannot set DataEnd */
			//USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
			//			USBC_EP_TYPE_EP0, 0);
			break;
		}
	}else{
		if (is_vendor_req || is_class_req) {
			sunxi_vendor_or_class_req(dev, crq);
			return;
		}

		//USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
		//			USBC_EP_TYPE_EP0, 0);
	}

	return;
}

static void sunxi_udc_handle_ep0(struct sunxi_udc *dev,
					  PUSBFNMP_MESSAGE message,
					  PUSBFNMP_MESSAGE_PAYLOAD payload)
{
	u32 ep0csr = 0;
	struct sunxi_udc_ep *ep = &dev->ep[0];
	struct sunxi_udc_request *req = NULL;
	USB_DEFAULT_PIPE_SETUP_PACKET crq;

	DMSG_DBG_UDC("sunxi_udc_handle_ep0--1--\n");

	if (list_empty(&ep->queue)) {
		req = NULL;
	} else {
		{
			const struct list_head *__mptr = (ep->queue.next);
			req = (struct sunxi_udc_request *)( (char *)__mptr
				- offsetof(struct sunxi_udc_request,queue) );
		}
	}

	DMSG_DBG_UDC("sunxi_udc_handle_ep0--2--\n");

	/*
	 * We make the assumption that sunxi_udc_UDC_IN_CSR1_REG equal to
	 * sunxi_udc_UDC_EP0_CSR_REG when index is zero.
	 */
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, 0);

	/* clear stall status */
	if (USBC_Dev_IsEpStall(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_EP0)) {
		DMSG_PANIC("ERR: ep0 stall\n");

		sunxi_udc_nuke(dev, ep, -EPIPE);
		USBC_Dev_EpClearStall(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_EP_TYPE_EP0);
		dev->ep0state = EP0_IDLE;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		return;
	}

	/* clear setup end */
	if (USBC_Dev_Ctrl_IsSetupEnd(g_sunxi_udc_io.usb_bsp_hdle)) {
		DMSG_PANIC("handle_ep0: ep0 setup end\n");

		sunxi_udc_nuke(dev, ep, 0);
		USBC_Dev_Ctrl_ClearSetupEnd(g_sunxi_udc_io.usb_bsp_hdle);
		dev->ep0state = EP0_IDLE;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
	}

	DMSG_DBG_UDC("sunxi_udc_handle_ep0--3--%d,%d\n", dev->ep0state,
		dev->ep0_data_trans);

	ep0csr = USBC_Readw( USBC_REG_CSR0((PUSHORT)g_sunxi_udc_io.usb_pbase));

	switch (dev->ep0state) {
	case EP0_IDLE:
		sunxi_udc_handle_ep0_idle(dev, ep, &crq, ep0csr, message,
			payload);
		break;
	case EP0_END_XFER:
		/* ep0 data transfer is going */
		if (dev->ep0_data_trans == EP0_DATA_TRANS_BUSY) {
			if (crq_dir == USB_DIR_IN) { /* GET_DESCRIPTOR etc */
				DMSG_DBG_UDC("EP0_END_XFER, in data\n");

				if (!USBC_Dev_IsWriteDataReady(
				    g_sunxi_udc_io.usb_bsp_hdle,
				    USBC_EP_TYPE_EP0) && req) {
					sunxi_udc_write_fifo(ep, req);
				}
			} else { /* SET_DESCRIPTOR etc */
				DMSG_DBG_UDC("EP0_END_XFER, out data\n");

				if (USBC_Dev_IsReadDataReady(
				    g_sunxi_udc_io.usb_bsp_hdle,
				    USBC_EP_TYPE_EP0) && req ) {
					sunxi_udc_read_fifo(ep,req);
				}
			}
			break;
		}

		switch (crq_bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			if (crq_dir == USB_DIR_IN) {
				*message = UsbMsgEndpointStatusChangedTx;
				payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			} else {
				*message = UsbMsgEndpointStatusChangedRx;
				payload->utr.Direction = UsbEndpointDirectionDeviceRx;
			}
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;
		case USB_REQ_GET_STATUS:
			if (crq_dir == USB_DIR_IN) {
				*message = UsbMsgEndpointStatusChangedTx;
				payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			} else {
				*message = UsbMsgEndpointStatusChangedRx;
				payload->utr.Direction = UsbEndpointDirectionDeviceRx;
			}
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;
		case VENDOR_REQ_WINUSB:
			if (crq_dir == USB_DIR_IN) {
				*message = UsbMsgEndpointStatusChangedTx;
				payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			} else {
				*message = UsbMsgEndpointStatusChangedRx;
				payload->utr.Direction = UsbEndpointDirectionDeviceRx;
			}
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;			
		}

		dev->ep0state = EP0_STATUS;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		break;
	case EP0_STATUS:
		DMSG_DBG_UDC("EP0_STATUS,crq_bRequest = 0x%x\n", crq_bRequest);

		switch (crq_bRequest) {
		case USB_REQ_SET_ADDRESS:
			*message = UsbMsgEndpointStatusChangedTx;
			payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;
		case USB_REQ_SET_CONFIGURATION:
			*message = UsbMsgEndpointStatusChangedTx;
			payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			/* rx_packet receive over */
			USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_EP0, 1);
			break;
		case USB_REQ_GET_DESCRIPTOR:
			*message = UsbMsgEndpointStatusChangedRx;
			payload->utr.Direction = UsbEndpointDirectionDeviceRx;
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;
		case USB_REQ_GET_STATUS:
			*message = UsbMsgEndpointStatusChangedRx;
			payload->utr.Direction = UsbEndpointDirectionDeviceRx;
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;
		case VENDOR_REQ_WINUSB:
			/* status stage dir is opposite to data stage dir */
			if (crq_dir == USB_DIR_IN) {
				*message = UsbMsgEndpointStatusChangedRx;
				payload->utr.Direction = UsbEndpointDirectionDeviceRx;
			} else {
				*message = UsbMsgEndpointStatusChangedTx;
				payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			}
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			break;
		case CLASS_REQ_SET_CONTROL_LINE_STATE:
			*message = UsbMsgEndpointStatusChangedTx;
			payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = EP0_INDEX;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			/* rx_packet receive over */
			USBC_Dev_ReadDataStatus(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_EP0, 1);
			break;
		}

		crq_bRequest = 0;
		crq_dir = 0;
		dev->ep0state = EP0_IDLE;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		break;
	}

	DMSG_DBG_UDC("sunxi_udc_handle_ep0--4--%d\n", dev->ep0state);
	return;
}

static void sunxi_udc_handle_ep(struct sunxi_udc_ep *ep,
				PUSBFNMP_MESSAGE message,
				PUSBFNMP_MESSAGE_PAYLOAD payload)
{
	struct sunxi_udc_request *req = NULL;
	int is_in = ep->bEndpointAddress & USB_DIR_IN;
	u32 idx = 0;
	u8 old_ep_index = 0;

	/* see sunxi_udc_queue. */
	if (!list_empty(&ep->queue)) {
		{
			const struct list_head *__mptr = (ep->queue.next);
			req = (struct sunxi_udc_request *)((char *)__mptr
				- offsetof(struct sunxi_udc_request,queue));
		}
	} else {
		req = NULL;
	}

	if(g_irq_debug){
		DMSG_INFO_UDC("e: (%d), tx_csr=0x%x\n", ep->num,
		 USBC_Readw(USBC_REG_TXCSR((PUSHORT)g_sunxi_udc_io.usb_pbase)));
		if(req){
			DMSG_INFO_UDC("req: (0x%p, %d, %d)\n", &(req),
				req->length, req->actual);
		}
	}

	idx = ep->bEndpointAddress & 0x7F;

	old_ep_index = (u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8) idx);

	if (is_in) {
		if(USBC_Dev_IsEpStall(g_sunxi_udc_io.usb_bsp_hdle,
		   USBC_EP_TYPE_TX)) {
			DMSG_PANIC("ERR: tx ep(%d) is stall\n", idx);
			USBC_Dev_EpClearStall(g_sunxi_udc_io.usb_bsp_hdle,
				USBC_EP_TYPE_TX);
			goto end;
		}
	} else {
		if (USBC_Dev_IsEpStall(g_sunxi_udc_io.usb_bsp_hdle,
		    USBC_EP_TYPE_RX)) {
			DMSG_PANIC("ERR: rx ep(%d) is stall\n", idx);
			USBC_Dev_EpClearStall(g_sunxi_udc_io.usb_bsp_hdle,
				USBC_EP_TYPE_RX);
			goto end;
		}
	}

	if (req) {
		if (is_in) {
			*message = UsbMsgEndpointStatusChangedTx;
			payload->utr.Direction = UsbEndpointDirectionDeviceTx;
			payload->utr.TransferStatus = UsbTransferStatusComplete;
			payload->utr.EndpointIndex = ep->num;
			payload->utr.BytesTransferred = req->actual;
			payload->utr.Buffer = req->buf;

			list_del_init(&req->queue);
			//if (!USBC_Dev_IsWriteDataReady(g_sunxi_udc_io.usb_bsp_hdle,
			//USBC_EP_TYPE_TX)) {
			//	sunxi_udc_write_fifo(ep, req);
			//}
		} else {
			if (USBC_Dev_IsReadDataReady(g_sunxi_udc_io.usb_bsp_hdle,
			    USBC_EP_TYPE_RX)) {
				if (sunxi_udc_read_fifo(ep,req)) {
					*message = UsbMsgEndpointStatusChangedRx;
					payload->utr.Direction = UsbEndpointDirectionDeviceRx;	
					payload->utr.TransferStatus = UsbTransferStatusComplete;
					payload->utr.EndpointIndex = ep->num;
					payload->utr.BytesTransferred = req->actual;
					payload->utr.Buffer = req->buf;

					list_del_init(&req->queue);
				}
			}
		}
	} else {
		DMSG_INFO_UDC("sunxi_udc_handle_ep: req is null\n");
	}

end:
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);
	return;
}

/* mask the useless irq, save disconect, reset, resume, suspend */
static u32 filtrate_irq_misc(u32 irq_misc)
{
	u32 irq = irq_misc;

	irq &= ~(USBC_INTUSB_VBUS_ERROR | USBC_INTUSB_SESSION_REQ
		| USBC_INTUSB_CONNECT | USBC_INTUSB_SOF);
	USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle, USBC_INTUSB_VBUS_ERROR);
	USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle, USBC_INTUSB_SESSION_REQ);
	USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle, USBC_INTUSB_CONNECT);
	USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle, USBC_INTUSB_SOF);

	return irq;
}

static void clear_all_irq(void)
{
	USBC_INT_ClearEpPendingAll(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_TX);
	USBC_INT_ClearEpPendingAll(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_RX);
	USBC_INT_ClearMiscPendingAll(g_sunxi_udc_io.usb_bsp_hdle);
}

static void throw_away_all_urb(struct sunxi_udc *dev)
{
	int k = 0;

	DMSG_INFO_UDC("irq: reset happen, throw away all urb\n");
	for(k = 0; k < SW_UDC_ENDPOINTS; k++){
		sunxi_udc_nuke(dev, (struct sunxi_udc_ep * )&(dev->ep[k]),
			-ECONNRESET);
	}
}

/* clear all dma status of the EP, called when dma exception */
static void sunxi_udc_clean_dma_status(struct sunxi_udc_ep *ep)
{
	u8 ep_index = 0;
	u8 old_ep_index = 0;
	struct sunxi_udc_request *req = NULL;

	ep_index = ep->bEndpointAddress & 0x7F;

	old_ep_index = USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, ep_index);

	if ((ep->bEndpointAddress) & USB_ENDPOINT_DIRECTION_MASK) { // dma_mode1
		/* clear ep dma status */
		USBC_Dev_ClearEpDma(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_TX);

		/* select bus to pio */
		sunxi_udc_switch_bus_to_pio(ep, 1);
	} else {  // dma_mode0
		/* clear ep dma status */
		USBC_Dev_ClearEpDma(g_sunxi_udc_io.usb_bsp_hdle, USBC_EP_TYPE_RX);

		/* select bus to pio */
		sunxi_udc_switch_bus_to_pio(ep, 0);
	}

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	/* done req */
	while(!list_empty(&ep->queue)) {
		{
			const struct list_head *__mptr = (ep->queue.next);
			req = (struct sunxi_udc_request *)((char *)__mptr
				- offsetof(struct sunxi_udc_request,queue));
		}

		if (req) {
			req->actual = 0;
			sunxi_udc_done(ep, req, -ECONNRESET);
		}
	}

	ep->dma_working = 0;
	dma_working = 0;

	return;
}

static void sunxi_udc_stop_dma_work(struct sunxi_udc *dev, u32 unlock)
{
	__u32 i = 0;
	struct sunxi_udc_ep *ep = NULL;

	for(i = 0; i < SW_UDC_ENDPOINTS; i++) {
		ep = &dev->ep[i];

		if (sunxi_udc_dma_is_busy(ep)) {
			DMSG_PANIC("wrn: ep(%d) must stop working\n", i);

			if (unlock) {
				sunxi_udc_dma_stop(ep);
			} else {
				sunxi_udc_dma_stop(ep);
			}

#ifdef SW_UDC_DMA_INNER
			ep->dev->dma_hdle = 0;
#else
			ep->dev->sunxi_udc_dma[ep->num].is_start = 0;
#endif
			ep->dma_transfer_len = 0;

			sunxi_udc_clean_dma_status(ep);
		}
	}

	return;
}

void sunxi_udc_dma_completion(struct sunxi_udc *dev,
				struct sunxi_udc_ep *ep,
				struct sunxi_udc_request *req,
				PUSBFNMP_MESSAGE message,
				PUSBFNMP_MESSAGE_PAYLOAD payload)
{
	__u8  			old_ep_index 		= 0;
	__u32 			dma_transmit_len	= 0;
	int 			is_complete		= 0;

	if (dev == NULL || ep == NULL || req == NULL) {
		DMSG_PANIC("ERR: argment invaild. (0x%p, 0x%p, 0x%p)\n",
			   dev, ep, req);
		return;
	}

	if (!ep->dma_working) {
		DMSG_PANIC("ERR: dma is not work, can not callback\n");
		return;
	}

	sunxi_udc_unmap_dma_buffer(req, dev, ep);

	old_ep_index = USBC_GetActiveEp(dev->sunxi_udc_io->usb_bsp_hdle);
	USBC_SelectActiveEp(dev->sunxi_udc_io->usb_bsp_hdle, ep->num);

	if ((ep->bEndpointAddress) & USB_DIR_IN) {  //tx, dma_mode1
		while(USBC_Dev_IsWriteDataReady_FifoEmpty(
			    dev->sunxi_udc_io->usb_bsp_hdle, USBC_EP_TYPE_TX));
			USBC_Dev_ClearEpDma(dev->sunxi_udc_io->usb_bsp_hdle,
					    USBC_EP_TYPE_TX);
	} else {  //rx, dma_mode0
		USBC_Dev_ClearEpDma(dev->sunxi_udc_io->usb_bsp_hdle,
				    USBC_EP_TYPE_RX);
	}

	dma_transmit_len = sunxi_udc_dma_transmit_length(ep,
					((ep->bEndpointAddress) & USB_DIR_IN),
					(__u32)req->buf);
	if (dma_transmit_len < req->length) {
		if ((ep->bEndpointAddress) & USB_DIR_IN) {
			USBC_Dev_ClearEpDma(dev->sunxi_udc_io->usb_bsp_hdle,
					    USBC_EP_TYPE_TX);
		} else {
			USBC_Dev_ClearEpDma(dev->sunxi_udc_io->usb_bsp_hdle,
					    USBC_EP_TYPE_RX);
		}
	}

	if (g_dma_debug) {
		DMSG_INFO_UDC("di: (0x%p, %d, %d)\n", req, req->length,
			req->actual);
	}

	ep->dma_working = 0;
	dma_working = 0;
	ep->dma_transfer_len = 0;

	/* if current data transfer not complete, then go on */
	req->actual += dma_transmit_len;
	if (req->length > req->actual) {
		if (((ep->bEndpointAddress & USB_DIR_IN) != 0)
			&& !USBC_Dev_IsWriteDataReady_FifoEmpty(
				dev->sunxi_udc_io->usb_bsp_hdle,
				USBC_EP_TYPE_TX)) {
			if (pio_write_fifo(ep, req)) {
				req = NULL;
				is_complete = 1;
			}
		} else if (((ep->bEndpointAddress & USB_DIR_IN) == 0)
			&& USBC_Dev_IsReadDataReady (
				dev->sunxi_udc_io->usb_bsp_hdle,
				USBC_EP_TYPE_RX)) {
			if (pio_read_fifo(ep, req)) {
				req = NULL;
				is_complete = 1;
			}
		}
	} else { /* if DMA transfer data over, then done */
		sunxi_udc_done(ep, req, 0);
		is_complete = 1;
	}

	/* start next transfer */
	if (is_complete) {
		if ((ep->bEndpointAddress & USB_DIR_IN) != 0) {
			*message = UsbMsgEndpointStatusChangedTx;
			payload->utr.Direction = UsbEndpointDirectionDeviceTx;
		} else {
			*message = UsbMsgEndpointStatusChangedRx;
			payload->utr.Direction = UsbEndpointDirectionDeviceRx;
		}
		payload->utr.TransferStatus = UsbTransferStatusComplete;
		payload->utr.EndpointIndex = ep->num;
		payload->utr.BytesTransferred = req->actual;
		payload->utr.Buffer = req->buf;

		list_del_init(&req->queue);
	}

	USBC_SelectActiveEp(dev->sunxi_udc_io->usb_bsp_hdle, old_ep_index);

	return;
}

int sunxi_udc_irq(PUSBFNMP_MESSAGE message,
		       PUSBFNMP_MESSAGE_PAYLOAD payload)
{
	struct sunxi_udc *dev = the_controller;
	struct sunxi_udc_ep *rx_ep = NULL;
	int usb_irq	= 0;
	int tx_irq	= 0;
	int rx_irq	= 0;
	int i		= 0;
	u32 old_ep_idx  = 0;
#ifdef SW_UDC_DMA_INNER
	int dma_irq	= 0;
#endif

	/* Driver connected ? */
	if (!is_peripheral_active()) {
		DMSG_PANIC("ERR: or udc is not active.\n");

		/* Clear interrupts */
		clear_all_irq();

		return IRQ_NONE;
	}

	/* Save index */
	old_ep_idx = USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);

	/* Read status registers */
	usb_irq = USBC_INT_MiscPending(g_sunxi_udc_io.usb_bsp_hdle);
	tx_irq  = USBC_INT_EpPending(g_sunxi_udc_io.usb_bsp_hdle,
				     USBC_EP_TYPE_TX);
	rx_irq  = USBC_INT_EpPending(g_sunxi_udc_io.usb_bsp_hdle,
				     USBC_EP_TYPE_RX);
#ifdef SW_UDC_DMA_INNER
	dma_irq = USBC_Readw(USBC_REG_DMA_INTS(
			     (PUSHORT)dev->sunxi_udc_io->usb_pbase));
#endif

	usb_irq = filtrate_irq_misc(usb_irq);

	if(g_irq_debug){
		DMSG_INFO_UDC("irq: usb_irq=%02x, tx_irq=%02x, rx_irq=%02x,%d\n",
			      usb_irq, tx_irq, rx_irq,dev->ep0state);
	}
	/*
	 * Now, handle interrupts. There's two types :
	 * - Reset, Resume, Suspend coming -> usb_int_reg
	 * - EP -> ep_int_reg
	 */

	/* EP */
	/* control traffic */
	/* check on ep0csr != 0 is not a good idea as clearing in_pkt_ready
	 * generate an interrupt
	 */
	if (tx_irq & USBC_INTTx_FLAG_EP0 || dev->ep0state != EP0_IDLE) {
		DMSG_DBG_UDC("USB ep0 irq\n");

		if (dev->speed == UsbBusSpeedUnknown) {
			if (USBC_Dev_QueryTransferMode(g_sunxi_udc_io.usb_bsp_hdle)
			    == USBC_TS_MODE_HS) {
				dev->speed = UsbBusSpeedHigh;
				*message = UsbMsgBusEventSpeed;
				payload->ubs = UsbBusSpeedHigh;

				DMSG_INFO_UDC("\n++++++++++++++++++++++++++\n");
				DMSG_INFO_UDC(" usb enter high speed.\n");
				DMSG_INFO_UDC("\n++++++++++++++++++++++++++\n");
			} else {
				dev->speed= UsbBusSpeedFull;
				*message = UsbMsgBusEventSpeed;
				payload->ubs = UsbBusSpeedFull;

				DMSG_INFO_UDC("\n++++++++++++++++++++++++++\n");
				DMSG_INFO_UDC(" usb enter full speed.\n");
				DMSG_INFO_UDC("\n++++++++++++++++++++++++++\n");
			}

			return IRQ_HANDLED;
		} else {
			if (dev->ep0state == EP0_IDLE)
				/* Clear the interrupt bit by setting it to 1 */
				USBC_INT_ClearEpPending(g_sunxi_udc_io.usb_bsp_hdle,
							USBC_EP_TYPE_TX, 0);
		}

		sunxi_udc_handle_ep0(dev, message, payload);

		return IRQ_HANDLED;
	}

	/* firstly to get data */

	/* rx endpoint data transfers */
	for (i = 1; i <= SW_UDC_EPNUMS; i++) {
	//for (i = 1; i < SW_UDC_ENDPOINTS; i++) {
		u32 tmp = 1 << i;

		if (rx_irq & tmp) {
			DMSG_DBG_UDC("USB rx ep%d irq\n", i);
			rx_ep = &dev->ep[ep_fifo_out[i]];

			if (list_empty(&rx_ep->queue)) {
				//LOG2("-emp--");
				continue;
			}

			/* Clear the interrupt bit by setting it to 1 */
			USBC_INT_ClearEpPending(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_RX, (__u8) i);

			sunxi_udc_handle_ep(&dev->ep[ep_fifo_out[i]], message,
					    payload);

			return IRQ_HANDLED;
		}
	}

	/* tx endpoint data transfers */
	for (i = 1; i <= SW_UDC_EPNUMS; i++) {
	//for (i = 1; i < SW_UDC_ENDPOINTS; i++) {
		u32 tmp = 1 << i;

		if (tx_irq & tmp) {
			DMSG_DBG_UDC("USB tx ep%d irq\n", i);

			/* Clear the interrupt bit by setting it to 1 */
			USBC_INT_ClearEpPending(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_TX, (__u8) i);

			sunxi_udc_handle_ep(&dev->ep[ep_fifo_in[i]], message,
					    payload);

			return IRQ_HANDLED;
		}
	}

#ifdef SW_UDC_DMA_INNER
	if(is_udc_support_dma()){
		struct sunxi_udc_request *req = NULL;
		struct sunxi_udc_ep *ep = NULL;

		/* tx endpoint data transfers */
		for (i = 0; i < DMA_CHAN_TOTAL; i++) {
			u32 tmp = 1 << i;

			if (dma_irq & tmp) {
				DMSG_DBG_UDC("USB dma chanle%d irq\n", i);

				/* set 1 to clear pending */
				writel(BIT(i), USBC_REG_DMA_INTS(
					(PULONG)dev->sunxi_udc_io->usb_pbase));
				//USBC_REG_clear_bit_w(tmp, USBC_REG_DMA_INTS(
				//dev->sunxi_udc_io->usb_vbase));

				ep = &dev->ep[dma_chnl[i].ep_num];
				sunxi_udc_dma_release((dm_hdl_t)ep->dev->dma_hdle);
				ep->dev->dma_hdle = 0;

				if (ep) {
					/* find req */
					if (!list_empty(&ep->queue)) {
						{
						const struct list_head *__mptr = (ep->queue.next);
						req = (struct sunxi_udc_request *)( (char *)__mptr
							- offsetof(struct sunxi_udc_request,queue) );
						}
					} else {
						req = NULL;
					}

					/* call back */
					if (req) {
						sunxi_udc_dma_completion(dev,
						     ep, req, message, payload);
					}
				} else {
					DMSG_PANIC("ERR: sunxi_udc_dma_callback: dma is remove, but dma irq is happened\n");
				}

				return IRQ_HANDLED;
			}
		}
	}
#endif

	/* RESET */
	if (usb_irq & USBC_INTUSB_RESET) {
		DMSG_INFO_UDC("IRQ: reset\n");

		USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_INTUSB_RESET);
		clear_all_irq();

		usb_connect = 1;

		USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, 0);
		USBC_Dev_SetAddress_default(g_sunxi_udc_io.usb_bsp_hdle);

		if (is_udc_support_dma()) {
			sunxi_udc_stop_dma_work(dev, 1);
		}

		throw_away_all_urb(dev);

		dev->address = 0;
		dev->ep0state = EP0_IDLE;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		//dev->gadget.speed = USB_SPEED_FULL;
		g_irq_debug = 0;
		g_queue_debug = 0;
		g_dma_debug = 0;

		*message = UsbMsgBusEventReset;

		goto done;
	}

	/* RESUME */
	if (usb_irq & USBC_INTUSB_RESUME) {
		DMSG_INFO_UDC("IRQ: resume\n");

		/* clear interrupt */
		USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_INTUSB_RESUME);
		*message = UsbMsgBusEventResume;

		goto done;
	}

	/* SUSPEND */
	if (usb_irq & USBC_INTUSB_SUSPEND) {
		DMSG_INFO_UDC("IRQ: suspend\n");

		/* clear interrupt */
		USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_INTUSB_SUSPEND);

		dev->ep0state = EP0_IDLE;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		*message = UsbMsgBusEventSuspend;

		goto done;
	}

	/* DISCONNECT */
	if (usb_irq & USBC_INTUSB_DISCONNECT) {
		DMSG_INFO_UDC("IRQ: disconnect\n");

		USBC_INT_ClearMiscPending(g_sunxi_udc_io.usb_bsp_hdle,
			USBC_INTUSB_DISCONNECT);

		dev->ep0state = EP0_IDLE;
		dev->ep0_data_trans = EP0_DATA_TRANS_IDLE;
		*message = UsbMsgBusEventDetach;
		usb_connect = 0;
	}

done:
	/* Restore old index */
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8) old_ep_idx);

	return IRQ_HANDLED;
}

static struct sunxi_udc_ep *get_sunxi_udc_ep(UINT8 ep_index,
				USBFNMP_ENDPOINT_DIRECTION dir)
{
	struct sunxi_udc_ep *ep = NULL;
	u8 ep_num = 0;
	struct sunxi_udc *dev = the_controller;

	if (ep_index == 0)
		ep_num = 0;
	else if (ep_index == 1 && dir == UsbEndpointDirectionDeviceTx)
		ep_num = 1;
	else if (ep_index == 1 && dir == UsbEndpointDirectionDeviceRx)
		ep_num = 2;
	else
		return NULL;
	ep = &dev->ep[ep_num];

	return ep;
}

int sunxi_udc_ep_enable(PUSB_ENDPOINT_DESCRIPTOR desc)
{
	__u32 fifo_addr	= 0;
	u8  ep_index	= 0;
	u32 ep_type	= 0;
	u32 ts_type	= 0;
	u32 fifo_size	= 0;
	u8  double_fifo	= 0;
	u32 old_ep_index		  = 0;
	USBFNMP_ENDPOINT_DIRECTION ep_dir = 0;
	struct sunxi_udc *dev		  = NULL;
	struct sunxi_udc_ep *ep		  = NULL;

	if(desc == NULL){
		DMSG_PANIC("ERR: invalid argment\n");
		return -EINVAL;
	}

	if (desc->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE) {
		DMSG_PANIC("PANIC : desc->bDescriptorType(%d) !="
			"USB_ENDPOINT_DESCRIPTOR_TYPE\n",
			desc->bDescriptorType);
		return -EINVAL;
	}

	ep_index = desc->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
	ep_dir = USB_ENDPOINT_DIRECTION_IN(desc->bEndpointAddress) ?
		 UsbEndpointDirectionHostIn : UsbEndpointDirectionHostOut;
	DMSG_INFO_UDC("ep enable: ep%d(%d, %d)\n", ep_index, ep_dir,
		       desc->wMaxPacketSize);
	ep = get_sunxi_udc_ep(ep_index, ep_dir);
	if (ep == NULL) {
		DMSG_PANIC("ERR: sunxi_udc_ep_enable: inval\n");
		return -EINVAL;
	}

	dev = ep->dev;
	if (dev->speed == UsbBusSpeedUnknown) {
		DMSG_PANIC("PANIC: dev->speed is USB_SPEED_UNKNOWN\n");
		return -ESHUTDOWN;
	}

	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

	//ep_type
	if (ep_dir)
		ep_type = USBC_EP_TYPE_TX;
	else
		ep_type = USBC_EP_TYPE_RX;

	//ts_type
	switch (desc->bmAttributes & USB_ENDPOINT_TYPE_MASK) {
	case USB_ENDPOINT_TYPE_CONTROL:
		ts_type   = USBC_TS_TYPE_CTRL;
		break;
	case USB_ENDPOINT_TYPE_BULK:
		ts_type   = USBC_TS_TYPE_BULK;
		break;
	case USB_ENDPOINT_TYPE_ISOCHRONOUS:
		ts_type   = USBC_TS_TYPE_ISO;
		break;
	case USB_ENDPOINT_TYPE_INTERRUPT:
		ts_type = USBC_TS_TYPE_INT;
		break;
	default:
		DMSG_PANIC("err: udc_ep_enable, unkown ep type(%d)\n",
			   (desc->bmAttributes & USB_ENDPOINT_TYPE_MASK));
		goto end;
	}

	if (!is_peripheral_active()) {
		DMSG_INFO_UDC("udc_ep_enable, usb device is not active\n");
		goto end;
	}

	if (desc->bEndpointAddress == 0x81) { /* tx */
		fifo_addr = ep_fifo[EP1IN_BULK].fifo_addr;
		fifo_size = SW_UDC_EP_FIFO_SIZE;
		double_fifo = ep_fifo[EP1IN_BULK].double_fifo;
	} else if (desc->bEndpointAddress == 0x01) { /* rx */
		fifo_addr = ep_fifo[EP1OUT_BULK].fifo_addr;
		fifo_size = SW_UDC_EP_FIFO_SIZE;
		double_fifo = ep_fifo[EP1OUT_BULK].double_fifo;
	} else {
		DMSG_PANIC("ERR: invalid ep addr\n");
		return -EINVAL;
	}

	old_ep_index = USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, ep_index);

	USBC_Dev_ConfigEp_Default(g_sunxi_udc_io.usb_bsp_hdle, ep_type);
	USBC_Dev_FlushFifo(g_sunxi_udc_io.usb_bsp_hdle, ep_type);

	//set max packet ,type, direction, address; reset fifo counters, enable irq
	USBC_Dev_ConfigEp(g_sunxi_udc_io.usb_bsp_hdle, ts_type, ep_type,
			  double_fifo, HIGH_SPEED_EP_MAX_PACKET_SIZE);
	USBC_ConfigFifo(g_sunxi_udc_io.usb_bsp_hdle, ep_type, double_fifo,
			fifo_size, fifo_addr);
	if(ts_type == USBC_TS_TYPE_ISO)
		USBC_Dev_IsoUpdateEnable(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_INT_EnableEp(g_sunxi_udc_io.usb_bsp_hdle, ep_type, ep_index);

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, (__u8) old_ep_index);

end:
	sunxi_udc_set_halt(ep_index, ep_dir, FALSE);

	return 0;
}

int sunxi_udc_queue(UINT8 ep_index, USBFNMP_ENDPOINT_DIRECTION dir,
		PULONG buf_size, PVOID buf, PHYSICAL_ADDRESS pa,
		ULONG buf_index)
{
	struct sunxi_udc_request *req = NULL;
	struct sunxi_udc_ep *ep = NULL;
	struct sunxi_udc *dev = the_controller;
	u8 old_ep_index = 0;

	ep = get_sunxi_udc_ep(ep_index, dir);
	if (ep == NULL) {
		DMSG_PANIC("ERR: sunxi_udc_queue: inval\n");
		return -EINVAL;
	}

	if (dev->speed == UsbBusSpeedUnknown) {
		DMSG_PANIC("ERR : dev->speed=%x\n", dev->speed);
		return -ESHUTDOWN;
	}

	req = &dev->fifo_req[buf_index];
	req->buf = buf;
	req->length = *buf_size;
	req->actual = 0;
	//req->dma_flag = 1;

	if (is_sunxi_udc_dma_capable(req, ep)) {
		sunxi_udc_map_dma_buffer(pa, req, ep);
	}

	list_add_tail(&req->queue, &ep->queue);

	if (!is_peripheral_active()) {
		DMSG_PANIC("warn: peripheral is active\n");
		goto end;
	}

	if(g_queue_debug){
		DMSG_INFO_UDC("q: (0x%p, %d, %d, %d)\n", ep, ep_index, dir,
			      buf_index);
	}

	old_ep_index = (u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	if (ep_index) {
		USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, ep_index);
	} else {
		USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, 0);
	}

	/* if there is only one in the queue, then execute it */
	if (!ep->halted && (&req->queue == ep->queue.next)) {
		if (ep_index == 0 /* ep0 */) {
			switch (dir) {
			case UsbEndpointDirectionDeviceTx:
				if (dev->ep0state == EP0_STATUS)
					break;
				if (!USBC_Dev_IsWriteDataReady(
						g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_EP0)
				    && sunxi_udc_write_fifo(ep, req)) {
					//dev->ep0state = EP0_IDLE;
					dev->ep0_data_trans
							= EP0_DATA_TRANS_IDLE;
					req = NULL;
				}
				break;
			case UsbEndpointDirectionDeviceRx:
				if (dev->ep0state == EP0_STATUS)
					break;
				if ((!req->length)
				    || (USBC_Dev_IsReadDataReady(
						g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_EP0)
				    && sunxi_udc_read_fifo(ep, req))) {
					//dev->ep0state = EP0_IDLE;
					dev->ep0_data_trans
							= EP0_DATA_TRANS_IDLE;
					req = NULL;
				}
				break;
			default:
				return -EL2HLT;
			}
		} else if (dir == UsbEndpointDirectionDeviceTx) {
			// && !USBC_Dev_IsWriteDataReady(g_sunxi_udc_io.usb_bsp_hdle,
			//				  USBC_EP_TYPE_TX)) {
			while (USBC_Dev_IsWriteDataReady(g_sunxi_udc_io.usb_bsp_hdle,
						  USBC_EP_TYPE_TX));
			if (sunxi_udc_write_fifo(ep, req)) {
				req = NULL;
			}
		} else if ((dir == UsbEndpointDirectionDeviceRx)
			   && USBC_Dev_IsReadDataReady(g_sunxi_udc_io.usb_bsp_hdle,
						       USBC_EP_TYPE_RX)) {
			//if (sunxi_udc_read_fifo(ep, req)) {
			//	req = NULL;
			//}
		}
	}

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

end:
	return 0;
}

/* dequeue JUST ONE request */
static int sunxi_udc_dequeue(UINT8 ep_index,
				      USBFNMP_ENDPOINT_DIRECTION dir)
{
	UNREFERENCED_PARAMETER(ep_index);
	UNREFERENCED_PARAMETER(dir);

	return 0;
}

int sunxi_udc_get_halt(u8 ep_num, USBFNMP_ENDPOINT_DIRECTION ep_dir,
			      PBOOLEAN stall_ep)
{
	__u8			old_ep_index = 0;

	if (!is_peripheral_active()) {
		DMSG_INFO_UDC("%s_%d: usb device is not active\n",
			__func__, __LINE__);
		return 0;
	}

	old_ep_index = (__u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, ep_num);

	if (ep_num == 0) {
		*stall_ep = (BOOLEAN) USBC_Dev_IsEpStall(g_sunxi_udc_io
					       .usb_bsp_hdle, USBC_EP_TYPE_EP0);
	} else {
		if (ep_dir) {
			*stall_ep = (BOOLEAN) USBC_Dev_IsEpStall(g_sunxi_udc_io
						.usb_bsp_hdle, USBC_EP_TYPE_TX);
		} else {
			*stall_ep = (BOOLEAN) USBC_Dev_IsEpStall(g_sunxi_udc_io
						.usb_bsp_hdle, USBC_EP_TYPE_RX);
		}
	}
	*stall_ep = *stall_ep ? 1 : 0;

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	return 0;	
}

int sunxi_udc_set_halt(u8 ep_index, USBFNMP_ENDPOINT_DIRECTION ep_dir,
			      BOOLEAN stall_ep)
{
	struct sunxi_udc_ep	*ep = NULL;
	__u8			old_ep_index = 0;

	ep = get_sunxi_udc_ep(ep_index, ep_dir);
	if (ep == NULL) {
		DMSG_PANIC("ERR: sunxi_udc_queue: inval\n");
		return -EINVAL;
	}
	if (!ep->desc && ep->num != EP0_INDEX) {
		DMSG_PANIC("ERR: !ep->desc && ep->ep.name != 0\n");
		return -EINVAL;
	}

	if (!is_peripheral_active()) {
		DMSG_INFO_UDC("%s_%d: usb device is not active\n",
			__func__, __LINE__);
		return 0;
	}

	old_ep_index = (u8) USBC_GetActiveEp(g_sunxi_udc_io.usb_bsp_hdle);
	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, ep_index);

	if (ep_index == 0) {
		USBC_Dev_EpClearStall(g_sunxi_udc_io.usb_bsp_hdle,
				      USBC_EP_TYPE_EP0);
	} else {
		if (ep_dir != 0) { /* tx */
			if (stall_ep) {
				USBC_Dev_EpSendStall(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_TX);
			} else {
				USBC_Dev_EpClearStall(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_TX);
			}
		} else { /* rx */
			if (stall_ep) {
				USBC_Dev_EpSendStall(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_RX);
			} else {
				USBC_Dev_EpClearStall(g_sunxi_udc_io.usb_bsp_hdle,
						USBC_EP_TYPE_RX);
			}
		}
	}

	ep->halted = stall_ep ? 1 : 0;

	USBC_SelectActiveEp(g_sunxi_udc_io.usb_bsp_hdle, old_ep_index);

	return 0;
}

static s32  usbd_start_work(void)
{
	DMSG_INFO_UDC("usbd_start_work\n");

	if (!is_peripheral_active()) {
		DMSG_INFO_UDC("%s_%d: usb device is not active\n",
			__func__, __LINE__);
		return 0;
	}

	enable_irq_udc(the_controller);
	USBC_Dev_ConectSwitch(g_sunxi_udc_io.usb_bsp_hdle,
		USBC_DEVICE_SWITCH_ON);
	return 0;
}

static s32  usbd_stop_work(void)
{
	DMSG_INFO_UDC("usbd_stop_work\n");

	if (!is_peripheral_active()) {
		DMSG_INFO_UDC("%s_%d: usb device is not active\n",
			__func__, __LINE__);
		return 0;
	}

	disable_irq_udc(the_controller);
	USBC_Dev_ConectSwitch(g_sunxi_udc_io.usb_bsp_hdle,
		USBC_DEVICE_SWITCH_OFF); // default is pulldown
	return 0;
}

static struct sunxi_udc sunxi_udc = {
	.ep[0] = {
		.num			= 0,
		.maxpacket		= EP0_FIFO_SIZE,
		.dev			= &sunxi_udc,
	},

	.ep[1] = {
		.num			= 1,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= (USB_DIR_IN | 1),
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[2] = {
		.num			= 1,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= (USB_DIR_OUT | 1),
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[3] = {
		.num			= 2,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= (USB_DIR_IN | 2),
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[4] = {
		.num			= 2,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= (USB_DIR_OUT | 2),
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[5] = {
		.num			= 3,
		.maxpacket		= SW_UDC_EP_ISO_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= 3,
		.bmAttributes		= USB_ENDPOINT_XFER_ISOC,
	},

	.ep[6] = {
		.num			= 4,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= 4,
		.bmAttributes		= USB_ENDPOINT_XFER_INT,
	},

#if defined (CONFIG_ARCH_SUN50IW1P1)
	.ep[7] = {
		.num			= 5,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= (USB_DIR_IN | 5),
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},

	.ep[8] = {
		.num			= 5,
		.maxpacket		= SW_UDC_EP_FIFO_SIZE,
		.dev		        = &sunxi_udc,
		//.fifo_size		= SW_UDC_EP_FIFO_SIZE,
		.bEndpointAddress	= (USB_DIR_OUT | 5),
		.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	},
#endif
};

static void cfg_udc_command(enum sunxi_udc_cmd_e cmd)
{
	switch (cmd)
	{
	case SW_UDC_P_ENABLE:
		{
			usbd_start_work();
		}
		break;
	case SW_UDC_P_DISABLE:
		{
			usbd_stop_work();
		}
		break;
	case SW_UDC_P_RESET :
		DMSG_PANIC("ERR: reset is not support\n");
		break;
	default:
		DMSG_PANIC("ERR: unkown cmd(%d)\n",cmd);
		break;
	}

	return ;
}

/*
 *	probe - binds to the platform device
 */
int sunxi_udc_probe(void)
{
	struct sunxi_udc *udc	= &sunxi_udc;

	the_controller = udc;

	sunxi_udc_reinit(udc);

	return 0;
}

