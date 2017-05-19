#ifndef _HWMAC_H
#define _HWMAC_H

/******************************************************************************
 *	Registers
 *****************************************************************************/

#define CCU_REGISTER_BASE	0x01C20000

#define AHB2_CLK_CFG		0x005C

#define CLK_GATING_REG0		0x0060
#define EMAC_GATING_OFFSET	17

#define SW_RESET_REG0		0x02C0
#define EMAC_RESET_OFFSET	17

#define PORT_CTRL_BASE		0x01C20800
#define PD_CFG_REG2_OFFSET	0x74
#define MDC_PORT_SEL		4 << 24
#define MDIO_PORT_SEL		4 << 28


#define GETH_BASIC_CTL0		0x00
#define GETH_BASIC_CTL1		0x04
#define GETH_INT_STA		0x08
#define GETH_INT_EN		0x0C
#define GETH_TX_CTL0		0x10
#define GETH_TX_CTL1		0x14
#define GETH_TX_FLOW_CTL	0x1C
#define GETH_TX_DESC_LIST	0x20
#define GETH_RX_CTL0		0x24
#define GETH_RX_CTL1		0x28
#define GETH_RX_DESC_LIST	0x34
#define GETH_RX_FRM_FLT		0x38
#define GETH_RX_HASH0		0x40
#define GETH_RX_HASH1		0x44
#define GETH_MDIO_ADDR		0x48
#define GETH_MDIO_DATA		0x4C
#define GETH_ADDR_HI(reg)	(0x50 + ((reg) << 3))
#define GETH_ADDR_LO(reg)	(0x54 + ((reg) << 3))
#define GETH_TX_DMA_STA		0xB0
#define GETH_TX_CUR_DESC	0xB4
#define GETH_TX_CUR_BUF		0xB8
#define GETH_RX_DMA_STA		0xC0
#define GETH_RX_CUR_DESC	0xC4
#define GETH_RX_CUR_BUF		0xC8
#define GETH_RGMII_STA		0xD0

// GETH_BASIC_CTL0		0x00
#define	CTL0_LM			0x02
#define CTL0_DM			0x01
#define CTL0_SPEED		0x04
#define CTL0_SPEED_1000M	0x0
#define CTL0_SPEED_100M		0xC
#define CTL0_SPEED_10M		0x8
#define CTL0_LOOPBACK_EN	0x2
#define CTL0_LOOPBACK_DIS	0x0
#define CTL0_DUPLEX_FULL	0x1
#define CTL0_DUPLEX_HALF	0x0

// GETH_BASIC_CTL1		0x04
#define BURST_LEN		0x3F000000
#define RX_TX_PRI		0x02
#define SOFT_RST		0x01

/* GETH_FRAME_FILTER 0x38  register value */
#define GETH_FRAME_FILTER_PR	0x00000001	/* Promiscuous Mode */
#define GETH_FRAME_FILTER_HUC	0x00000002	/* Hash Unicast */
#define GETH_FRAME_FILTER_HMC	0x00000004	/* Hash Multicast */
#define GETH_FRAME_FILTER_DAIF	0x00000008	/* DA Inverse Filtering */
#define GETH_FRAME_FILTER_PM	0x00000010	/* Pass all multicast */
#define GETH_FRAME_FILTER_DBF	0x00000020	/* Disable Broadcast frames */
#define GETH_FRAME_FILTER_SAIF	0x00000100	/* Inverse Filtering */
#define GETH_FRAME_FILTER_SAF	0x00000200	/* Source Address Filter */
#define GETH_FRAME_FILTER_HPF	0x00000400	/* Hash or perfect Filter */
#define GETH_FRAME_FILTER_RA	0x80000000	/* Receive all mode */

#define RGMII_IRQ		0x00000001

typedef struct _HW_MAC
{
	PVOID RegisterBase;
	PHY Phy;
}MAC, *PMAC;

struct _ADAPTER_HW;

NDIS_STATUS HW_MAC_Init(struct _ADAPTER_HW *PhyAdapter, PMAC Mac);
VOID HW_Mac_Read(PMAC Mac, ULONG Address, PULONG value);
VOID HW_Mac_Write(PMAC Mac, ULONG Address, ULONG value);
NDIS_STATUS HW_Mac_Reset(PMAC Mac, ULONG delayus);
NDIS_STATUS HW_Mac_Wait_Reset_Clear(PMAC Mac, ULONG delayus);
NDIS_STATUS HW_Mac_Set_Mode(PMAC Mac, ULONG mode);
NDIS_STATUS HW_Mac_Config_Transmit(PMAC Mac, ULONG txmode, ULONG rxmode);
NDIS_STATUS HW_Mac_Set_Hash_Filter(PMAC Mac, ULONG low, ULONG high);
NDIS_STATUS HW_Mac_Set_Filter(PMAC Mac, ULONG flags);
NDIS_STATUS HW_Mac_Set_Mac_Address(PMAC Mac, unsigned char *addr, ULONG index);
NDIS_STATUS HW_Mac_Enable(PMAC Mac);
NDIS_STATUS HW_Mac_Disable(PMAC Mac);
VOID HW_MAC_Start_Stop_DMA(PMAC Mac, BOOLEAN Start, BOOLEAN Tx, ULONG DmaAddr);
VOID HW_MAC_Start_DMA_Transfer(PMAC Mac, BOOLEAN Tx);
NDIS_STATUS HW_MAC_Set_Mdio_Pin_Function(struct _ADAPTER_HW *PhyAdapter);

__forceinline
VOID HW_Mac_Int_Enable(PMAC Mac)
{
	HW_Mac_Write(Mac, GETH_INT_EN, (RX_INT | TX_INT | TX_UNF_INT));	
	return;
}

__forceinline
VOID HW_Mac_Int_Disable(PMAC Mac)
{
	HW_Mac_Write(Mac, GETH_INT_EN, 0);
	return;
}


__forceinline
VOID HW_Mac_Get_Int_Status(PMAC Mac, ULONG *Int)
{
	HW_Mac_Read(Mac, GETH_INT_STA, Int);	
	return;
}

__forceinline
VOID HW_Mac_Clear_Int(PMAC Mac, ULONG Int)
{
	HW_Mac_Write(Mac, GETH_INT_STA, (Int & 0x3FFF));	
	return;
}

#endif

