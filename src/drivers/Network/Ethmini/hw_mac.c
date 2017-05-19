#include "Ethmini.h"
#include "hw_mac.tmh"

NDIS_STATUS HW_MAC_Init(PHWADAPTER PhyAdapter, PMAC Mac)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	
    DEBUGP(MP_TRACE, "HW_MAC_Init ---> \n");
	
	Mac->RegisterBase = PhyAdapter->Register.VirtualBase;
	ASSERT(Mac->RegisterBase);

	//Set MDIO & MDC GPIO to right function
	CHKSTATUS(HW_MAC_Set_Mdio_Pin_Function(PhyAdapter), 0);
	
	// Wait HW reset is finished
	CHKSTATUS(HW_Mac_Wait_Reset_Clear(Mac, 100), 0);

	// Init power and clock here, we already enable in UEFI
	// move code here when system power management enabled.
	
	CHKSTATUS(HW_Init_Phy_Interface(Mac, &Mac->Phy), 0);
	CHKSTATUS(HW_Mac_Reset(Mac, 100), 0);
	 //ToDO, Get LINK mode from Registry
	HW_Mac_Set_Mode(Mac, CTL0_SPEED_100M | CTL0_LOOPBACK_DIS | CTL0_DUPLEX_FULL);
	HW_Mac_Config_Transmit(Mac, SF_DMA_MODE, SF_DMA_MODE);
	HW_Mac_Set_Mac_Address(Mac, PhyAdapter->Adapter->CurrentAddress, 0);
	HW_Mac_Set_Filter(Mac, 0
		|GETH_FRAME_FILTER_RA  | GETH_FRAME_FILTER_PR 			// All
		);

	//HW_Mac_Enable(Mac);
	

Exit:
	DEBUGP(MP_TRACE, "[%p] <--- HW_MAC_Init Status = 0x%x\n", PhyAdapter->Adapter, Status);
	DbgPrintEx(0,0, "[%p] <--- HW_MAC_Init Status = 0x%x\n", PhyAdapter->Adapter, Status);
	
	return Status;
}

__forceinline
VOID HW_Mac_Read(PMAC Mac, ULONG Address, PULONG value)
{
	*value = READ_REGISTER_ULONG((PULONG)((ULONG)Mac->RegisterBase + Address));
	return;
}

__forceinline
VOID HW_Mac_Write(PMAC Mac, ULONG Address, ULONG value)
{
	WRITE_REGISTER_ULONG((PULONG)((ULONG)Mac->RegisterBase + Address), value);
	return;
}


NDIS_STATUS HW_Mac_Reset(PMAC Mac, ULONG delayus)
{
	ULONG							value;
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

	/* MAC Interface SW reset */
	HW_Mac_Read(Mac, GETH_BASIC_CTL1, &value);
	value |= SOFT_RST;
	HW_Mac_Write(Mac, GETH_BASIC_CTL1, value);

	NdisMSleep(delayus);

	HW_Mac_Read(Mac, GETH_BASIC_CTL1, &value);

	if(value & SOFT_RST)
	{

		Status = NDIS_STATUS_FAILURE;
	}

	return Status;
}

NDIS_STATUS HW_Mac_Wait_Reset_Clear(PMAC Mac, ULONG delayus)
{
	ULONG							value = 0;
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

	/* MAC Interface SW reset */
	HW_Mac_Read(Mac, GETH_BASIC_CTL1, &value);
	if(!(value & SOFT_RST))
	{

		goto Exit;
	}

	NdisMSleep(delayus);

	value = 0;
	HW_Mac_Read(Mac, GETH_BASIC_CTL1, &value);

	if(value & SOFT_RST)
	{

		Status = NDIS_STATUS_HARD_ERRORS;
	}
	
Exit:
	return Status;
}

NDIS_STATUS HW_Mac_Set_Mode(PMAC Mac, ULONG mode)
{
	ULONG							value, valueback;
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

	/* MAC Interface SW reset */
	HW_Mac_Read(Mac, GETH_BASIC_CTL0, &value);
	value |= mode;
	HW_Mac_Write(Mac, GETH_BASIC_CTL0, value);
	HW_Mac_Read(Mac, GETH_BASIC_CTL0, &valueback);
	if(value != valueback)
	{
		//__debugbreak();

	}

	return Status;
}

NDIS_STATUS HW_Mac_Config_Transmit(PMAC Mac, ULONG txmode, ULONG rxmode)
{	
	ULONG							value;
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;

	/* Burst should be 8 */
	value = (8 << 24);
	value |= RX_TX_PRI;	/* Rx has priority over tx */
	HW_Mac_Write(Mac, GETH_BASIC_CTL1, value);

	/* Mask interrupts by writing to CSR7 */
	value = (RX_INT | TX_INT | TX_UNF_INT);
	// HW_Mac_Write(Mac, GETH_INT_EN, value);

	/* Initialize the core component */
	HW_Mac_Read(Mac, GETH_TX_CTL0, &value);
	value |= (1 << 30);	/* Jabber Disable */
	HW_Mac_Write(Mac, GETH_TX_CTL0, value);

	HW_Mac_Read(Mac, GETH_RX_CTL0, &value);
	value |= (1 << 27);	/* Enable CRC & IPv4 Header Checksum */
	value |= (1 << 28);	/* Automatic Pad/CRC Stripping */
	value |= (1 << 29);	/* Jumbo Frame Enable */
	HW_Mac_Write(Mac, GETH_RX_CTL0, value);

	//writel((hwdev.mdc_div << 20), iobase + GETH_MDIO_ADDR); /* MDC_DIV_RATIO */

	/* Set the Rx&Tx mode */
	HW_Mac_Read(Mac, GETH_TX_CTL1, &value);
	if (txmode == SF_DMA_MODE) {
		/* Transmit COE type 2 cannot be done in cut-through mode. */
		value |= TX_MD;
		/* Operating on second frame increase the performance
		 * especially when transmit store-and-forward is used.*/
		value |= TX_NEXT_FRM;
	} else {
		value &= ~TX_MD;
		value &= ~TX_TH;
		/* Set the transmit threshold */
		if (txmode <= 64)
			value |= 0x00000000;
		else if (txmode <= 128)
			value |= 0x00000100;
		else if (txmode <= 192)
			value |= 0x00000200;
		else
			value |= 0x00000300;
	}
	HW_Mac_Write(Mac, GETH_TX_CTL1, value);

	HW_Mac_Read(Mac, GETH_RX_CTL1, &value);
	if (rxmode == SF_DMA_MODE) {
		value |= RX_MD;
	} else {
		value &= ~RX_MD;
		value &= ~RX_TH;
		if (rxmode <= 32)
			value |= 0x10;
		else if (rxmode <= 64)
			value |= 0x00;
		else if (rxmode <= 96)
			value |= 0x20;
		else
			value |= 0x30;
	}
	HW_Mac_Write(Mac, GETH_RX_CTL1, value);

	return Status;
}

NDIS_STATUS HW_Mac_Set_Hash_Filter(PMAC Mac, ULONG low, ULONG high)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	HW_Mac_Write(Mac, GETH_RX_HASH0, high);
	HW_Mac_Write(Mac, GETH_RX_HASH1, low);
	return Status;
}

NDIS_STATUS HW_Mac_Set_Filter(PMAC Mac, ULONG flags)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;
	ULONG 							value;

	HW_Mac_Read(Mac, GETH_RX_FRM_FLT, &value);

	value |= ((flags >> 31) |
			((flags >> 9) & 0x00000002) |
			((flags << 1) & 0x00000010) |
			((flags >> 3) & 0x00000060) |
			((flags << 7) & 0x00000300) |
			((flags << 6) & 0x00003000) |
			((flags << 12) & 0x00030000) |
			(flags << 31));

	HW_Mac_Write(Mac, GETH_RX_FRM_FLT, value);
	
	return Status;
}

NDIS_STATUS HW_Mac_Set_Mac_Address(PMAC Mac, unsigned char *addr, ULONG index)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;
	ULONG 							value;

	value = (addr[5] << 8) | addr[4];
	HW_Mac_Write(Mac, GETH_ADDR_HI(index), value);
	value = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	HW_Mac_Write(Mac, GETH_ADDR_LO(index), value);
	
	return Status;
}

NDIS_STATUS HW_Mac_Enable(PMAC Mac)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;
	ULONG value;

	HW_Mac_Read(Mac, GETH_TX_CTL0, &value);
	value |= (1 << 31);
	HW_Mac_Write(Mac, GETH_TX_CTL0, value);

	HW_Mac_Read(Mac, GETH_RX_CTL0, &value);
	value |= (1 << 31);
	HW_Mac_Write(Mac, GETH_RX_CTL0, value);
	
	return Status;
}

NDIS_STATUS HW_Mac_Disable(PMAC Mac)
{
	NDIS_STATUS 					Status = NDIS_STATUS_SUCCESS;

	unsigned long value;

	HW_Mac_Read(Mac, GETH_TX_CTL0, &value);
	value &= ~(1 << 31);
	HW_Mac_Write(Mac, GETH_TX_CTL0, value);

	HW_Mac_Read(Mac, GETH_RX_CTL0, &value);
	value &= ~(1 << 31);
	HW_Mac_Write(Mac, GETH_RX_CTL0, value);
	return Status;
}

VOID HW_MAC_Start_Stop_DMA(PMAC Mac, BOOLEAN Start, BOOLEAN Tx, ULONG DmaAddr)
{
	ULONG DmaDescAddr;
	ULONG TransCtlAddr;
	ULONG value;

	DmaDescAddr = Tx ? GETH_TX_DESC_LIST : GETH_RX_DESC_LIST;
	TransCtlAddr = Tx ? GETH_TX_CTL1 : GETH_RX_CTL1;
	
	HW_Mac_Read(Mac, TransCtlAddr, &value);
	if(Start)
	{
		HW_Mac_Write(Mac, DmaDescAddr, DmaAddr);
		value |= 0x40000000;
	}
	else
	{
		value &= ~0x40000000;
	}
	HW_Mac_Write(Mac, TransCtlAddr, value);
}

VOID HW_MAC_Start_DMA_Transfer(PMAC Mac, BOOLEAN Tx)
{
	ULONG TransCtlAddr;
	ULONG value;

	TransCtlAddr = Tx ? GETH_TX_CTL1 : GETH_RX_CTL1;
	
	HW_Mac_Read(Mac, TransCtlAddr, &value);
	value |= 0x80000000;
	HW_Mac_Write(Mac, TransCtlAddr, value);
}


NDIS_STATUS HW_MAC_Set_Mdio_Pin_Function(PHWADAPTER PhyAdapter)
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

	ULONG value;
	ULONG newValue;

	value = READ_REGISTER_ULONG((PULONG)PhyAdapter->MdioPinRegister.VirtualBase);

	newValue = value & 0xFFFFFF | MDC_PORT_SEL | MDIO_PORT_SEL;

	WRITE_REGISTER_ULONG((PULONG)PhyAdapter->MdioPinRegister.VirtualBase, newValue);

	return Status;
}

