#include "Ethmini.h"
#include "hw_phy.tmh"

NDIS_STATUS HW_Init_Phy_Interface(PMAC Mac, PHY * Phy)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

	Phy->InterfaceType = MII_INTERFACE;

	Phy->Interface.mii.Command = (PVOID)((ULONG)Mac->RegisterBase + GETH_MDIO_ADDR);
	Phy->Interface.mii.Data = (PVOID)((ULONG)Mac->RegisterBase + GETH_MDIO_DATA);
	Phy->Interface.mii.Mdcdiv = MDC_DIV_DEFAULT;

	HW_Phy_Enumerate_Phy(Phy);
	CHKSTATUS(HW_Phy_Get_Default_ID(Phy, &Phy->PhyInUse), 0);
	CHKSTATUS(HW_Phy_Reset(Phy, Phy->PhyInUse), "PHY Reset Failure\n");
	CHKSTATUS(HW_Phy_Set_Mode(Phy, Phy->PhyInUse), 0);
	//CHKSTATUS(HW_Phy_Wait_Link_Up(Phy, Phy->PhyInUse), 0);
	HW_Phy_Dump_Status(Phy, Phy->PhyInUse);

Exit:
	return Status;
}

VOID HW_Phy_Check_Link_Up(PHY *Phy, PHYID PhyId, BOOLEAN* LinkUp)
{
	ULONG value;

	HW_Phy_Read(Phy, PhyId, PHY_BASIC_STA_REG, &value);
	if (value & BASIC_STA_LINK_STATUS)
	{
		*LinkUp = TRUE;
	}
	else
	{
		*LinkUp = FALSE;
	}
	
}

NDIS_STATUS HW_Phy_Enumerate_Phy(PHY *Phy)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;
	ULONG							index;
	ULONG                           value = 0;
	ULONG							value2;

	for (index = 0; index < SUPPORTED_PHYS; index++)
	{
		value = 0;
		
		HW_Phy_Read(Phy, index, PHY_ID1_REG, &value);
		if ((value != 0) && (value != 0xFFFF))
		{
			HW_Phy_Read(Phy, index, PHY_ID2_REG, &value2);
			Phy->PhyDev[index].Id = ((value & 0xFFFF) << 6) | ((value2 >> 10) & 0x3F);
			Phy->PhyDev[index].Model = ((value2 >> 4) & 0x3F);
			Phy->PhyDev[index].Rev = ((value2) & 0xF);
		}
	}

	return Status;
}


NDIS_STATUS HW_Phy_Get_Default_ID(PHY *Phy, PHYID * PhyID)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;
	ULONG							index;

	// Find the first Phy Device
	for (index = 0; index < SUPPORTED_PHYS; index++)
	{
		if (Phy->PhyDev[index].Id != 0)
		{
			break;
		}
	}

	if (index >= SUPPORTED_PHYS)
	{
		index = INVALID_ID;
		Status = NDIS_STATUS_FAILURE;
	}

	*PhyID = index;

	return Status;
}

NDIS_STATUS HW_Phy_Set_PhyID(PHY *Phy, PHYID PhyID)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;

	if (Phy->PhyDev[PhyID].Id != 0)
	{
		Phy->PhyInUse = PhyID;
	}
	else
	{
		Status = NDIS_STATUS_DEVICE_FAILED;
	}

	return Status;
}



NDIS_STATUS HW_Phy_Reset(PHY *Phy, PHYID PhyID)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG							value;
	ULONG							wait = 10;

	// Write BMCR control to reset PHY
	HW_Phy_Write(Phy, PhyID, PHY_BASIC_CTL_REG, BMCR_RESET);

	HW_Phy_Read(Phy, PhyID, PHY_BASIC_CTL_REG, &value);
	while ((value & BMCR_RESET) && wait)
	{
		NdisMSleep(1000);
		HW_Phy_Read(Phy, PhyID, PHY_BASIC_CTL_REG, &value);
		wait--;
	}

	if (value & BMCR_RESET)
	{
		Status = NDIS_STATUS_FAILURE;
		DbgPrintEx(0, 0, "HW_PHY_Reset fail Status = 0x%x\n", Status);
	}

	return Status;
}

NDIS_STATUS HW_Phy_Set_Mode(PHY *Phy, PHYID PhyID)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG							value;

	// Write BMCR control to select mode
	// loopback en, Auto Nego en, 100Base-T Duplex
	value = BASIC_CTL_100BASET
		//| BASIC_CTL_LOOPBACK 
		//| BASIC_CTL_AUTONEG 
		//| BASIC_CTL_ANRESTART 
		| BASIC_CTL_FULLDUP;
	HW_Phy_Write(Phy, PhyID, PHY_BASIC_CTL_REG, value);

	return Status;
}

NDIS_STATUS HW_Phy_Wait_Link_Up(PHY *Phy, PHYID PhyID)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG							value;
	ULONG							wait = 1000;

	HW_Phy_Read(Phy, PhyID, PHY_BASIC_STA_REG, &value);
	while (((value & BASIC_STA_LINK_STATUS) == 0) && wait)
	{
		NdisMSleep(100 * 1000);	// 100 mili seconds
		HW_Phy_Read(Phy, PhyID, PHY_BASIC_STA_REG, &value);
		wait--;
	}

	if ((value & BASIC_STA_LINK_STATUS) == 0)
	{
		Status = NDIS_STATUS_FAILURE;
		//__debugbreak();
		goto Exit;
	}

	wait = 1000;
	HW_Phy_Read(Phy, PhyID, PHY_SPEC_STA_REG, &value);
	while (((value & SPEC_STA_LINK_STATUS) == 0) && wait)
	{
		NdisMSleep(100 * 1000); // 100 mili seconds
		HW_Phy_Read(Phy, PhyID, PHY_SPEC_STA_REG, &value);
		wait--;
	}

	if ((value & SPEC_STA_LINK_STATUS) == 0)
	{
		Status = NDIS_STATUS_FAILURE;
		//__debugbreak();
		goto Exit;
	}

Exit:
	return Status;
}

NDIS_STATUS HW_Phy_Dump_Status(PHY *Phy, PHYID PhyID)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG							value;

	HW_Phy_Read(Phy, PhyID, PHY_BASIC_STA_REG, &value);
	HW_Phy_Read(Phy, PhyID, PHY_SPEC_STA_REG, &value);

	return Status;
}




NDIS_STATUS HW_Phy_Set_Div(PHY *Phy)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

	if (Phy->InterfaceType == MII_INTERFACE)
	{

	}

	return Status;
}


NDIS_STATUS HW_Phy_Read(PHY *Phy, PHYID PhyID, ULONG Address, PULONG value)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG							Command;

	if (Phy->InterfaceType == MII_INTERFACE)
	{
		Command = PHY_COMMAND(Phy->Interface.mii.Mdcdiv, PhyID, Address, MII_READ);
		while (READ_REGISTER_ULONG(Phy->Interface.mii.Command)&MII_BUSY);
		WRITE_REGISTER_ULONG(Phy->Interface.mii.Command, Command);
		while (READ_REGISTER_ULONG(Phy->Interface.mii.Command)&MII_BUSY);
		*value = READ_REGISTER_ULONG(Phy->Interface.mii.Data);
	}
	else
	{
		Status = NDIS_STATUS_DEVICE_FAILED;
	}

	return Status;
}

NDIS_STATUS HW_Phy_Write(PHY *Phy, PHYID PhyID, ULONG Address, ULONG value)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG							Command;

	if (Phy->InterfaceType == MII_INTERFACE)
	{
		Command = PHY_COMMAND(Phy->Interface.mii.Mdcdiv, PhyID, Address, MII_WRITE);
		while (READ_REGISTER_ULONG(Phy->Interface.mii.Command)&MII_BUSY);
		WRITE_REGISTER_ULONG(Phy->Interface.mii.Data, value);
		WRITE_REGISTER_ULONG(Phy->Interface.mii.Command, Command);
		while (READ_REGISTER_ULONG(Phy->Interface.mii.Command)&MII_BUSY);
	}
	else
	{
		Status = NDIS_STATUS_DEVICE_FAILED;
	}

	return Status;
}

