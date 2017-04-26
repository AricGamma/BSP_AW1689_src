#ifndef _HWDMA_H
#define _HWDMA_H


#define SF_DMA_MODE		1

#define TX_FLUSH		0x01
#define TX_MD			0x02
#define TX_NEXT_FRM		0x04
#define TX_TH			0x0700

#define RX_FLUSH		0x01
#define RX_MD			0x02
#define RX_RUNT_FRM		0x04
#define RX_TH			0x0030

#define TX_INT			0x00001
#define TX_STOP_INT		0x00002
#define TX_UA_INT		0x00004
#define TX_TOUT_INT		0x00008
#define TX_UNF_INT		0x00010
#define TX_EARLY_INT		0x00020
#define RX_INT			0x00100
#define RX_UA_INT		0x00200
#define RX_STOP_INT		0x00400
#define RX_TOUT_INT		0x00800
#define RX_OVF_INT		0x01000
#define RX_EARLY_INT		0x02000
#define LINK_STA_INT		0x10000

#define DISCARD_FRAME	-1
#define GOOD_FRAME	0
#define CSUM_NONE	2
#define LLC_SNAP	4

/* Default tx descriptor */
#define TX_SINGLE_DESC0		0x80000000
#define TX_SINGLE_DESC1		0x63000000

/* Default rx descriptor */
#define RX_SINGLE_DESC0		0x80000000
#define RX_SINGLE_DESC1		0x83000000

#define DMA_HW_OWN			0x80000000
#define DMA_RX_ERROR_BITS	0x68DB//0xF8CB
#define DMA_TX_ERROR_BITS	0x15707


typedef union {
	struct {
		/* TDES0 */
		ULONG deferred:1;	/* Deferred bit (only half-duplex) */
		ULONG under_err:1;	/* Underflow error */
		ULONG ex_deferral:1;	/* Excessive deferral */
		ULONG coll_cnt:4;	/* Collision count */
		ULONG vlan_tag:1;	/* VLAN Frame */
		ULONG ex_coll:1;		/* Excessive collision */
		ULONG late_coll:1;	/* Late collision */
		ULONG no_carr:1;		/* No carrier */
		ULONG loss_carr:1;	/* Loss of collision */
		ULONG ipdat_err:1;	/* IP payload error */
		ULONG frm_flu:1;		/* Frame flushed */
		ULONG jab_timeout:1;	/* Jabber timeout */
		ULONG err_sum:1;		/* Error summary */
		ULONG iphead_err:1;	/* IP header error */
		ULONG ttss:1;		/* Transmit time stamp status */
		ULONG reserved0:13;
		ULONG own:1;		/* Own bit. CPU:0, DMA:1 */
	} tx;

	/* bits 5 7 0 | Frame status
	 * ----------------------------------------------------------
	 *      0 0 0 | IEEE 802.3 Type frame (length < 1536 octects)
	 *      1 0 0 | IPv4/6 No CSUM errorS.
	 *      1 0 1 | IPv4/6 CSUM PAYLOAD error
	 *      1 1 0 | IPv4/6 CSUM IP HR error
	 *      1 1 1 | IPv4/6 IP PAYLOAD AND HEADER errorS
	 *      0 0 1 | IPv4/6 unsupported IP PAYLOAD
	 *      0 1 1 | COE bypassed.. no IPv4/6 frame
	 *      0 1 0 | Reserved.
	 */
	struct {
		/* RDES0 */
		ULONG chsum_err:1;	/* Payload checksum error */
		ULONG crc_err:1;		/* CRC error */
		ULONG dribbling:1;	/* Dribble bit error */
		ULONG mii_err:1;		/* Received error (bit3) */
		ULONG recv_wt:1;		/* Received watchdog timeout */
		ULONG frm_type:1;	/* Frame type */
		ULONG late_coll:1;	/* Late Collision */
		ULONG ipch_err:1;	/* IPv header checksum error (bit7) */
		ULONG last_desc:1;	/* Laset descriptor */
		ULONG first_desc:1;	/* First descriptor */
		ULONG vlan_tag:1;	/* VLAN Tag */
		ULONG over_err:1;	/* Overflow error (bit11) */
		ULONG len_err:1;		/* Length error */
		ULONG sou_filter:1;	/* Source address filter fail */
		ULONG desc_err:1;	/* Descriptor error */
		ULONG err_sum:1;		/* Error summary (bit15) */
		ULONG frm_len:14;	/* Frame length */
		ULONG des_filter:1;	/* Destination address filter fail */
		ULONG own:1;		/* Own bit. CPU:0, DMA:1 */
	#define RX_PKT_OK		0x7FFFB77C
	#define RX_LEN			0x3FFF0000
	} rx;

	unsigned int all;
} desc0_u;

typedef union {
	struct {
		/* TDES1 */
		ULONG buf1_size:11;	/* Transmit buffer1 size */
		ULONG buf2_size:11;	/* Transmit buffer2 size */
		ULONG ttse:1;		/* Transmit time stamp enable */
		ULONG dis_pad:1;		/* Disable pad (bit23) */
		ULONG adr_chain:1;	/* Second address chained */
		ULONG end_ring:1;	/* Transmit end of ring */
		ULONG crc_dis:1;		/* Disable CRC */
		ULONG cic:2;		/* Checksum insertion control (bit27:28) */
		ULONG first_sg:1;	/* First Segment */
		ULONG last_seg:1;	/* Last Segment */
		ULONG interrupt:1;	/* Interrupt on completion */
	} tx;

	struct {
		/* RDES1 */
		ULONG buf1_size:11;	/* Received buffer1 size */
		ULONG buf2_size:11;	/* Received buffer2 size */
		ULONG reserved1:2;
		ULONG adr_chain:1;	/* Second address chained */
		ULONG end_ring:1;		/* Received end of ring */
		ULONG reserved2:5;
		ULONG dis_ic:1;		/* Disable interrupt on completion */
	} rx;

	unsigned int all;
} desc1_u;

#pragma pack(1)
typedef struct _DMA_DESC {
	desc0_u desc0;
	desc1_u desc1;
	/* The address of buffers */
	ULONG	desc2;
	/* Next desc's addresss */
	ULONG	desc3;
} DMA_DESC, *PDMA_DESC;
#pragma pack()

//NDIS_STATUS HW_DMA_Init(PMAC Mac);
void HW_DMA_Init_Desc_Chain(PDMA_DESC desc, ULONG addr, ULONG size);

__forceinline
VOID HW_DMA_Desc_Reuse(PDMA_DESC desc)
{
	desc->desc2  = 0;
	desc->desc1.all = 0;
	desc->desc1.all |= (1 << 24);
}


__forceinline
VOID HW_DMA_Set_Buffer(PDMA_DESC desc, ULONG addr, ULONG size)
{
	desc->desc1.all &= (~((1<<11) - 1));
	desc->desc1.all |= (size & ((1<<11) - 1));
	desc->desc2 = addr;
}

__forceinline
VOID HW_DMA_Set_Own(PDMA_DESC desc)
{
	desc->desc0.all |= 0x80000000;
}

__forceinline
VOID HW_DMA_Clear_Status(PDMA_DESC desc)
{
	desc->desc0.all &= 0x80000000;
}


__forceinline
VOID HW_DMA_Set_Rx_Int(PDMA_DESC desc)
{
	desc->desc1.all |= 0x80000000;
}


__forceinline
VOID HW_DMA_Set_Tx_First(PDMA_DESC desc)
{
	desc->desc1.tx.first_sg = 1;
}

__forceinline
VOID HW_DMA_Set_Tx_Last(PDMA_DESC desc)
{
	desc->desc1.tx.last_seg = 1;
	desc->desc1.tx.interrupt = 1;
	//desc->desc1.tx.dis_pad = 1;
	//desc->desc1.tx.cic = 3;
	//desc->desc1.tx.crc_dis = 1;
}

__forceinline
ULONG HW_DMA_Get_Owner_Bit(PDMA_DESC desc)
{
	return desc->desc0.all & 0x80000000;
}

__forceinline
ULONG HW_DMA_Get_Status(PDMA_DESC desc)
{
	return desc->desc0.all;
}

__forceinline
ULONG HW_DMA_Get_Rx_Len(PDMA_DESC desc)
{
	return desc->desc0.rx.frm_len;
}


#endif
