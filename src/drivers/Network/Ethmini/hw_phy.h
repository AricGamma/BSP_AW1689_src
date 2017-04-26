#ifndef _HWPHY_H
#define _HWPHY_H

#define PHY_BASIC_CTL_REG				0x0
#define PHY_BASIC_STA_REG				0x1
#define PHY_ID1_REG 					0x2
#define PHY_ID2_REG 					0x3
#define PHY_AN_AD_REG					0x4
#define PHY_LINK_PART_REG				0x5
#define PHY_1000M_CTL_REG				0x9
#define PHY_1000M_STA_REG				0xA
#define PHY_SPEC_CTL_REG				0x10
#define PHY_SPEC_STA_REG				0x11
#define PHY_RX_ERR_CNT_REG				0x18

#define PHY_PAGE_SEL_REG				31
#define PHY_PAGE44_REG_30				30
#define PHY_PAGE44_REG_26				26
#define PHY_PAGE44_REG_28				28


// PHY_BASIC_CTL_REG 0
#define BMCR_RESET 0x8000

#define BASIC_CTL_RESET		0x8000
#define BASIC_CTL_LOOPBACK	0x4000
#define BASIC_CTL_100BASET	0x2000
#define BASIC_CTL_AUTONEG	0x1000
#define BASIC_CTL_PWD		0x0800
#define BASIC_CTL_ISO		0x0400
#define BASIC_CTL_ANRESTART	0x0200
#define BASIC_CTL_FULLDUP	0x0100
#define BASIC_CTL_COLTEST	0x0080
#define BASIC_CTL_SPEED_RES	0x0040

// PHY_BASIC_CTL_REG	1
#define BASIC_STA_LINK_STATUS	0x0004

// PHY_SPEC_STA_REG		0x11
#define SPEC_STA_LINK_STATUS	0x0400	

// MAC register GETH_MDIO_ADDR		0x48
#define MII_BUSY		0x00000001
#define MII_WRITE		0x00000002
#define MII_READ		0x00000000
#define MII_PHY_MASK	0x0000FFC0
#define MII_CR_MASK		0x0000001C
#define MII_CLK			0x00000008
/* bits 4 3 2 | AHB1 Clock	| MDC Clock
 * -------------------------------------------------------
 *      0 0 0 | 60 ~ 100 MHz	| div-42
 *      0 0 1 | 100 ~ 150 MHz	| div-62
 *      0 1 0 | 20 ~ 35 MHz	| div-16
 *      0 1 1 | 35 ~ 60 MHz	| div-26
 *      1 0 0 | 150 ~ 250 MHz	| div-102
 *      1 0 1 | 250 ~ 300 MHz	| div-124
 *      1 1 x | Reserved	|
 */

#define PHY_COMMAND(mdc_div, dev, reg, write)\
	 ( (((mdc_div) & 0x7) << 20) |\
	    (((dev) << 12) & 0x0001F000) |\
	    (((reg) << 4) & (0x000007F0)) |\
	    ((write) & 0x2)|\
	    MII_BUSY)


#define MDC_DIV_DEFAULT  0x3

#define SUPPORTED_PHYS   32
#define INVALID_ID       0xFFFFFFFF


typedef struct _MII
{
	PVOID Command;
	PVOID Data;
	ULONG Mdcdiv;
}MII;

typedef struct _GMII
{
	PVOID Status;
}GMII;

typedef enum _PHY_TYPE
{
	MII_INTERFACE,
	GMII_INTERFACE
}PHY_TYPE;

typedef ULONG PHYID;

typedef struct _PHY_MODEL
{
	PHYID Id;
	ULONG Model;
	ULONG Rev;

}PHY_MODEL;

typedef struct _HW_PHY
{
	union
	{
		MII mii;
		GMII gmii;
	}Interface;
	PHY_TYPE InterfaceType;
	PHYID PhyInUse;
	PHY_MODEL PhyDev[SUPPORTED_PHYS];		
}PHY, *PPHY;

struct _HW_MAC;

NDIS_STATUS HW_Init_Phy_Interface(struct _HW_MAC *Mac, PHY * Phy);
NDIS_STATUS HW_Phy_Enumerate_Phy(PHY *Phy);
NDIS_STATUS HW_Phy_Get_Default_ID(PHY *Phy, PHYID * PhyID);
NDIS_STATUS HW_Phy_Set_PhyID(PHY *Phy, PHYID PhyID);
NDIS_STATUS HW_Phy_Reset(PHY *Phy, PHYID PhyID);
NDIS_STATUS HW_Phy_Set_Div(PHY *Phy);
NDIS_STATUS HW_Phy_Read(PHY *Phy, PHYID PhyID, ULONG Address, PULONG value);
NDIS_STATUS HW_Phy_Write(PHY *Phy, PHYID PhyID, ULONG Address, ULONG value);
NDIS_STATUS HW_Phy_Set_Mode(PHY *Phy, PHYID PhyID);
NDIS_STATUS HW_Phy_Wait_Link_Up(PHY *Phy, PHYID PhyID);
NDIS_STATUS HW_Phy_Dump_Status(PHY *Phy, PHYID PhyID);

#endif
