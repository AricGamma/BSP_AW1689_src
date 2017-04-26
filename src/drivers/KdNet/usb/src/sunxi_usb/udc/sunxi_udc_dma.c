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

#include  "sunxi_udc_config.h"
#include  "sunxi_udc_board.h"
#include  "sunxi_udc_dma.h"

dma_channel_t dma_chnl[DMA_CHAN_TOTAL];

unsigned long dma_map_single(volatile void *vaddr, size_t len,
					   enum dma_data_direction dir)
{
	UNREFERENCED_PARAMETER(len);
	UNREFERENCED_PARAMETER(dir);

	return (unsigned long)vaddr;
}

void dma_unmap_single(volatile void *vaddr, size_t len,
				    unsigned long paddr)
{
	UNREFERENCED_PARAMETER(vaddr);
	UNREFERENCED_PARAMETER(len);
	UNREFERENCED_PARAMETER(paddr);
}

void dma_sync_single_for_device(dma_addr_t addr, size_t size,
					   enum dma_data_direction dir)
{
	UNREFERENCED_PARAMETER(addr);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(dir);
}

void dma_sync_single_for_cpu(dma_addr_t addr, size_t size,
					enum dma_data_direction dir)
{
	UNREFERENCED_PARAMETER(addr);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(dir);
}

/* switch usb bus for dma */
void sunxi_udc_switch_bus_to_dma(struct sunxi_udc_ep *ep, u32 is_tx)
{
	UNREFERENCED_PARAMETER(ep);
	UNREFERENCED_PARAMETER(is_tx);

	return;
}

/* switch usb bus for pio */
void sunxi_udc_switch_bus_to_pio(struct sunxi_udc_ep *ep, __u32 is_tx)
{
	UNREFERENCED_PARAMETER(ep);
	UNREFERENCED_PARAMETER(is_tx);

	return;
}

/* enable dma channel irq */
void sunxi_udc_enable_dma_channel_irq(struct sunxi_udc_ep *ep)
{
	UNREFERENCED_PARAMETER(ep);

	DMSG_DBG_DMA("sunxi_udc_enable_dma_channel_irq\n");
	return;
}

/* disable dma channel irq */
void sunxi_udc_disable_dma_channel_irq(struct sunxi_udc_ep *ep)
{
	UNREFERENCED_PARAMETER(ep);
}

#ifdef SW_UDC_DMA_INNER
dm_hdl_t sunxi_udc_dma_request(void)
{
	int i = 0;
	dma_channel_t *pchan = NULL;

	/* get a free channel */
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &dma_chnl[i];
		if (0 == pchan->used) {
			pchan->used = 1;
			pchan->channel_num = i;
			return (dm_hdl_t)pchan;
		}
	}

	return (dm_hdl_t)NULL;
}

int sunxi_udc_dma_release(dm_hdl_t dma_hdl)
{
	dma_channel_t *pchan = NULL;
	u32 reg_value = 0;

	if (dma_hdl == NULL) {
		DMSG_PANIC("ERR: sunxi_udc_dma_release failed dma_hdl is NULL\n");
		return -1;
	}

	pchan = (dma_channel_t *)dma_hdl;
	reg_value = USBC_Readw(USBC_REG_DMA_INTE(pchan->reg_base));
	reg_value &= ~(1 << (pchan->channel_num & 0xff));
	USBC_Writew((USHORT)reg_value, USBC_REG_DMA_INTE(pchan->reg_base));

	pchan->used = 0;
	pchan->channel_num = 0;

	return 0;
}

/* config dma */
void sunxi_dma_set_config(dm_hdl_t dma_hdl, struct dma_config_t *pcfg)
{
	u16 reg_value = 0;
	dma_channel_t *pchan = NULL;

	if (dma_hdl == NULL || pcfg == NULL) {
		DMSG_PANIC("ERR: sunxi_dma_set_config failed dma_hdl or pcfg is NULL\n");
		return;
	}

	pchan = (dma_channel_t *)dma_hdl;

	reg_value = (u16)USBC_Readl(USBC_REG_DMA_CHAN_CFN(pchan->reg_base,
			       pcfg->dma_num));
	reg_value &= ~((1 << 4) | (0xf << 0) | (0x7ff << 16));

	//eplen
	reg_value |=  (((pcfg->dma_bst_len) & 0x7ff) << 16);

	//DIR
	reg_value |= ((pcfg->dma_dir & 1) << 4);

	//ep num
	reg_value |=  ((pcfg->dma_for_ep & 0xf) << 0);
	USBC_Writel(reg_value, USBC_REG_DMA_CHAN_CFN(pchan->reg_base,
		    pcfg->dma_num));

	//address
	USBC_Writel(pcfg->dma_sdram_str_addr, USBC_REG_DMA_SDRAM_ADD(
		    pchan->reg_base, pcfg->dma_num));

	//transport len
	USBC_Writel((pcfg->dma_bc), USBC_REG_DMA_BC(pchan->reg_base,
		    pcfg->dma_num));

	reg_value = USBC_Readw(USBC_REG_DMA_INTE(pchan->reg_base));
	reg_value |= (1 << (pcfg->dma_num & 0xff));
	USBC_Writew(reg_value, USBC_REG_DMA_INTE(pchan->reg_base));

	/* start dma */
	reg_value = (u16)USBC_Readl(USBC_REG_DMA_CHAN_CFN(pchan->reg_base,
			       pcfg->dma_num));
	reg_value |=  (1U << 31);
	USBC_Writel(reg_value, USBC_REG_DMA_CHAN_CFN(pchan->reg_base,
		    pcfg->dma_num));
}

void sunxi_udc_dma_set_config(struct sunxi_udc_ep *ep,
					 struct sunxi_udc_request *req,
					 __u32 buff_addr, __u32 len)
{
	UNREFERENCED_PARAMETER(req);

	dm_hdl_t dma_hdl = NULL;
	dma_channel_t *pchan = NULL;
	__u32 is_tx = 0;
	__u32 packet_size = 0;

	struct dma_config_t DmaConfig;

	memset(&DmaConfig, 0, sizeof(DmaConfig));

	is_tx = is_tx_ep(ep);
	packet_size = ep->maxpacket;

	dma_hdl = sunxi_udc_dma_request();
	if (dma_hdl == NULL) {
		DMSG_PANIC("ERR: sunxi_udc_dma_request failed dma_hdl is NULL\n");
		return;
	}

	ep->dev->dma_hdle = (int)dma_hdl;
	pchan = (dma_channel_t *)dma_hdl;
	if (is_tx)
		pchan->ep_num = ep_fifo_in[ep->num];
	else /* rx */
		pchan->ep_num = ep_fifo_out[ep->num];

	pchan->reg_base = (u32 __force)ep->dev->sunxi_udc_io->usb_pbase,

	DmaConfig.dma_bst_len = packet_size;
	DmaConfig.dma_dir = !is_tx;
	DmaConfig.dma_for_ep = ep->num;
	DmaConfig.dma_bc = len;
	DmaConfig.dma_sdram_str_addr = buff_addr;
	DmaConfig.dma_num = pchan->channel_num;

	sunxi_dma_set_config(dma_hdl, &DmaConfig);
}

/* start dma transfer */
void sunxi_udc_dma_start(struct sunxi_udc_ep *ep, __u32 fifo,
				 __u32 buffer, __u32 len)
{
	UNREFERENCED_PARAMETER(ep);
	UNREFERENCED_PARAMETER(fifo);
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(len);
}

/* stop dma transfer */
void sunxi_udc_dma_stop(struct sunxi_udc_ep *ep)
{
	UNREFERENCED_PARAMETER(ep);
}

/* query the length that has been transferred */
__u32 sunxi_udc_dma_transmit_length(struct sunxi_udc_ep *ep,
						__u32 is_in, __u32 buffer_addr)
{
	UNREFERENCED_PARAMETER(is_in);
	UNREFERENCED_PARAMETER(buffer_addr);

	return ep->dma_transfer_len;
}

/* check if dma busy */
__u32 sunxi_udc_dma_is_busy(struct sunxi_udc_ep *ep)
{
	return ep->dma_working;
}

/* dma initilize */
__s32 sunxi_udc_dma_probe(struct sunxi_udc *dev)
{
	UNREFERENCED_PARAMETER(dev);

	return 0;
}

/* dma remove */
__s32 sunxi_udc_dma_remove(struct sunxi_udc *dev)
{
	UNREFERENCED_PARAMETER(dev);

	return 0;
}
#endif /* SW_UDC_DMA_INNER */

