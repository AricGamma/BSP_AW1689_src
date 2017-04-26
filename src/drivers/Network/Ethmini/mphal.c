/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   MpHAL.C

Abstract:

    This module implements the adapter's hardware.

--*/


#include "Ethmini.h"
#include "mphal.tmh"

//
// This registry value saves the permanent MAC address of a netvmini NIC.  It is
// only needed because there's no hardware that keeps track of the permanent
// address across reboots.
//
// It is saved as a NdisParameterBinary configuration value (REG_BINARY) with
// length NIC_MACADDR_LEN (6 bytes).
//
#define NETVMINI_MAC_ADDRESS_KEY L"NetvminiMacAddress"


static
NDIS_STATUS
HWCopyBytesFromNetBuffer(
    _In_     PNET_BUFFER        NetBuffer,
    _Inout_  PULONG             cbDest,
    _Out_writes_bytes_to_(*cbDest, *cbDest) PVOID Dest);


#pragma NDIS_PAGEABLE_FUNCTION(HWInitialize)
#pragma NDIS_PAGEABLE_FUNCTION(HWReadPermanentMacAddress)

// Allocate Phy Adapter
NDIS_STATUS AllocPhyAdapter
(_In_  PMP_ADAPTER                     Adapter,
_Outptr_ PHWADAPTER                    *pPhyAdapter)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	ULONG                           Size;
	PHWADAPTER                      PhyAdapter = NULL;

	Size = sizeof(HWADAPTER);
	
	PhyAdapter = NdisAllocateMemoryWithTagPriority(
                NdisDriverHandle,
                Size,
                NIC_TAG,
                NormalPoolPriority);
	if (!PhyAdapter)
    {
        Status = NDIS_STATUS_RESOURCES;
        DEBUGP(MP_ERROR, "Failed to allocate memory for physical adapter.\n");
        goto Exit;
    }

	NdisZeroMemory(PhyAdapter, Size);
	PhyAdapter->Size = Size;
	PhyAdapter->Adapter = Adapter;

Exit:
	*pPhyAdapter = PhyAdapter;
	return Status;
}

// Free Phy Adapter
NDIS_STATUS FreePhyAdapter
(_In_  PHWADAPTER                   pPhyAdapter)
{
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	
	if(pPhyAdapter)
	{
		pPhyAdapter->Adapter = NULL;
		NdisFreeMemory(pPhyAdapter, pPhyAdapter->Size, 0);
	}
	
	return Status;
}

NDIS_STATUS
HWInitialize(
    _In_  PMP_ADAPTER                     Adapter,
    _In_  PNDIS_MINIPORT_INIT_PARAMETERS  InitParameters)
/*++
Routine Description:

    Query assigned resources and initialize the adapter.

Arguments:

    Adapter                     Pointer to our adapter
    InitParameters              Parameters to MiniportInitializeEx

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_ADAPTER_NOT_FOUND

--*/
{
	NDIS_STATUS                     Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDesc;
    ULONG                           index;
	BOOLEAN                         bPorttFound = FALSE;
	BOOLEAN                         bInterruptFound = FALSE;
	BOOLEAN                         bMemoryFound = FALSE;
	PHWADAPTER                      PhyAdapter = NULL;

    DEBUGP(MP_TRACE, "[%p] ---> HWInitialize\n", Adapter);
    PAGED_CODE();
	
	// Allocate Hardware Adapter
	Status = AllocPhyAdapter(Adapter, &PhyAdapter);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		goto Exit;
	}
	
    do
    {
        if (InitParameters->AllocatedResources)
        {
            for (index=0; index < InitParameters->AllocatedResources->Count; index++)
            {
                pResDesc = &InitParameters->AllocatedResources->PartialDescriptors[index];

                switch (pResDesc->Type)
                {
                    case CmResourceTypePort:
                        DEBUGP(MP_INFO, "[%p] IoBaseAddress = 0x%x\n", Adapter,
                                NdisGetPhysicalAddressLow(pResDesc->u.Port.Start));
                        DEBUGP(MP_INFO, "[%p] IoRange = x%x\n", Adapter, 
                                pResDesc->u.Port.Length);
						PhyAdapter->rPort = *pResDesc;
						bPorttFound = TRUE;
                        break;

                    case CmResourceTypeInterrupt:
                        DEBUGP(MP_INFO, "[%p] InterruptLevel = x%x\n", Adapter,
                                pResDesc->u.Interrupt.Level);
						PhyAdapter->rInterrupt = *pResDesc;
						bInterruptFound = TRUE;
                        break;

                    case CmResourceTypeMemory:
                        DEBUGP(MP_INFO, "[%p] MemPhysAddress(Low) = 0x%0x\n", Adapter,
                                NdisGetPhysicalAddressLow(pResDesc->u.Memory.Start));
                        DEBUGP(MP_INFO, "[%p] MemPhysAddress(High) = 0x%0x\n", Adapter,
                                NdisGetPhysicalAddressHigh(pResDesc->u.Memory.Start));
						PhyAdapter->rMemory = *pResDesc;
						bMemoryFound = TRUE;
                        break;
                }
            }
        }

        Status = NDIS_STATUS_SUCCESS;

        //
        // Map bus-relative IO range to system IO space using
        // NdisMRegisterIoPortRange
        //
        if(bPorttFound)
    	{
    	}

        //
        // Map bus-relative registers to virtual system-space
        // using NdisMMapIoSpace
        //
        if(bMemoryFound)
    	{
    		PhyAdapter->Register.PhysicalBase = PhyAdapter->rMemory.u.Memory.Start;
			PhyAdapter->Register.Length = PhyAdapter->rMemory.u.Memory.Length;
    		Status = NdisMMapIoSpace(
				&PhyAdapter->Register.VirtualBase,
				Adapter->AdapterHandle,
				PhyAdapter->Register.PhysicalBase,
				PhyAdapter->Register.Length);
			if(Status != NDIS_STATUS_SUCCESS)
			{
				DEBUGP(MP_ERROR, "Failed to map physcial memory space.\n");
        		goto Exit;
			}
			//
			// Initialize the hardware with mapped resources
			//

			// Init HW MAC and PHY interface
			Status = HW_MAC_Init(PhyAdapter, &PhyAdapter->Mac);
			if(Status != NDIS_STATUS_SUCCESS)
			{
				DEBUGP(MP_ERROR, "Failed to Init Hardware.\n");
	    		goto Exit;
			}
    	}

		if(bInterruptFound)
    	{
    	    //
	        // Disable interrupts here as soon as possible
	        //
	        HwDisableInterrupt(PhyAdapter);

	        //
	        // Register the interrupt using NdisMRegisterInterruptEx
	        //
			Status = HwRegisterInterrupt(PhyAdapter);
			if(Status != NDIS_STATUS_SUCCESS)
			{
				DEBUGP(MP_ERROR, "Failed to register interrupt routine.\n");
        		goto Exit;
			}

			//
        	// Enable the interrupt
       		//
        	HwEnableInterrupt(PhyAdapter);
    	}
    } while (FALSE);

Exit:

    DEBUGP(MP_TRACE, "[%p] <--- HWInitialize Status = 0x%x\n", Adapter, Status);
	if(Status != NDIS_STATUS_SUCCESS)
	{
		if(PhyAdapter)
		{
			FreePhyAdapter(PhyAdapter);
			PhyAdapter = NULL;
		}
	}
	
	Adapter->PhyAdapter = PhyAdapter;
    return Status;
}


VOID
HWReadPermanentMacAddress(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_HANDLE  ConfigurationHandle,
    _Out_writes_bytes_(NIC_MACADDR_SIZE)  PUCHAR  PermanentMacAddress)
/*++

Routine Description:

    Loads the permanent MAC address that is burnt into the NIC.

    IRQL = PASSIVE_LEVEL

Arguments:

    Adapter                     Pointer to our adapter
    ConfigurationHandle         NIC configuration from NdisOpenConfigurationEx
    PermanentMacAddress         On return, receives the NIC's MAC address

Return Value:

    None.

--*/
{
    NDIS_STATUS                   Status;
    PNDIS_CONFIGURATION_PARAMETER Parameter = NULL;
    NDIS_STRING                   PermanentAddressKey = RTL_CONSTANT_STRING(NETVMINI_MAC_ADDRESS_KEY);
 
    UNREFERENCED_PARAMETER(Adapter);
    PAGED_CODE();

    //
    // We want to figure out what the NIC's physical address is.
    // If we had a hardware NIC, we would query the physical address from it.
    // Instead, for the purposes of this sample, we'll read it from the
    // registry.  This will help us keep the permanent address constant, even
    // if the adapter is disabled/enabled.
    //
    // Note that the registry value that saves our permanent MAC address isn't
    // the one that end-users can configure through the NIC management GUI,
    // nor is it expected that other miniports would need to use a parameter
    // like this.  We only have it to work around the lack of physical hardware.
    //
    NdisReadConfiguration(
            &Status,
            &Parameter,
            ConfigurationHandle,
            &PermanentAddressKey,
            NdisParameterBinary);
    if (Status == NDIS_STATUS_SUCCESS
            && Parameter->ParameterType == NdisParameterBinary
            && Parameter->ParameterData.BinaryData.Length == NIC_MACADDR_SIZE)
    {
        //
        // There is a permanent address stashed in the special netvmini
        // parameter.
        //
        NIC_COPY_ADDRESS(PermanentMacAddress, Parameter->ParameterData.BinaryData.Buffer);
    }
    else
    {
        NDIS_CONFIGURATION_PARAMETER NewPhysicalAddress;
        LARGE_INTEGER TickCountValue;
        UCHAR CurrentMacIndex = 3;

        //
        // There is no (valid) address stashed in the netvmini parameter, so
        // this is probably the first time we've loaded this adapter before.
        //
        // Just for testing purposes, let us make up a dummy mac address.
        // In order to avoid conflicts with MAC addresses, it is usually a good
        // idea to check the IEEE OUI list (e.g. at
        // http://standards.ieee.org/regauth/oui/oui.txt). According to that
        // list 00-50-F2 is owned by Microsoft.
        //
        // An important rule to "generating" MAC addresses is to have the
        // "locally administered bit" set in the address, which is bit 0x02 for
        // LSB-type networks like Ethernet. Also make sure to never set the
        // multicast bit in any MAC address: bit 0x01 in LSB networks.
        //

        {C_ASSERT(NIC_MACADDR_SIZE > 3);}
        NdisZeroMemory(PermanentMacAddress, NIC_MACADDR_SIZE);
        PermanentMacAddress[0] = 0x00;
        PermanentMacAddress[1] = 0x11;
        PermanentMacAddress[2] = 0xce;

        //
        // Generated value based on the current tick count value. 
        //
        KeQueryTickCount(&TickCountValue);
        do
        {
            //
            // Pick up the value in groups of 8 bits to populate the rest of the MAC address.
            //
            PermanentMacAddress[CurrentMacIndex] = (UCHAR)(TickCountValue.LowPart>>((CurrentMacIndex-3)*8));
        } while(++CurrentMacIndex < NIC_MACADDR_SIZE);

        //
        // Finally, we should make a best-effort attempt to save this address
        // to our configuration, so the NIC will always come up with this
        // permanent address.
        //

        NewPhysicalAddress.ParameterType = NdisParameterBinary;
        NewPhysicalAddress.ParameterData.BinaryData.Length = NIC_MACADDR_SIZE;
        NewPhysicalAddress.ParameterData.BinaryData.Buffer = PermanentMacAddress;

        NdisWriteConfiguration(
                &Status,
                ConfigurationHandle,
                &PermanentAddressKey,
                &NewPhysicalAddress);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(MP_WARNING, "[%p] NdisWriteConfiguration failed to save the permanent MAC address", Adapter);
            // No other handling -- this isn't a fatal error
        }
    }
}

NDIS_STATUS
HWCopyBytesFromNetBuffer(
    _In_     PNET_BUFFER        NetBuffer,
    _Inout_  PULONG             cbDest,
    _Out_writes_bytes_to_(*cbDest, *cbDest) PVOID Dest)
/*++

Routine Description:

    Copies the first cbDest bytes from a NET_BUFFER. In order to show how the various data structures fit together, this 
    implementation copies the data by iterating through the MDLs for the NET_BUFFER. The NdisGetDataBuffer API also allows you
    to copy a contiguous block of data from a NET_BUFFER. 

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    NetBuffer                   The NB to read
    cbDest                      On input, the number of bytes in the buffer Dest
                                On return, the number of bytes actually copied
    Dest                        On return, receives the first cbDest bytes of
                                the network frame in NetBuffer

Return Value:

    None.

Notes:

    If the output buffer is larger than the NB's frame size, *cbDest will
    contain the number of bytes in the frame size.

    If the output buffer is smaller than the NB's frame size, only the first
    *cbDest bytes will be copied (the buffer will receive a truncated copy of
    the frame).

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    //
    // Start copy from current MDL
    //
    PMDL CurrentMdl = NET_BUFFER_CURRENT_MDL(NetBuffer);
    //
    // Data on current MDL may be offset from start of MDL
    //
    ULONG DestOffset = 0;
    while (DestOffset < *cbDest && CurrentMdl)
    {
        //
        // Map MDL memory to System Address Space. LowPagePriority means mapping may fail if 
        // system is low on memory resources. 
        //
        PUCHAR SrcMemory = MmGetSystemAddressForMdlSafe(CurrentMdl, LowPagePriority);
        ULONG Length = MmGetMdlByteCount(CurrentMdl);
        if (!SrcMemory)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        if(DestOffset==0)
        {
            //
            // The first MDL segment should be accessed from the current MDL offset
            //
            ULONG MdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer);
            SrcMemory += MdlOffset;
            Length -= MdlOffset;
        }

        Length = min(Length, *cbDest-DestOffset);

        //
        // Copy Memory
        //
        NdisMoveMemory((PUCHAR)Dest+DestOffset, SrcMemory, Length);
        DestOffset += Length;

        //
        // Get next MDL (if any available) 
        //
        CurrentMdl = NDIS_MDL_LINKAGE(CurrentMdl);
    }

    if(Status == NDIS_STATUS_SUCCESS)
    {
        *cbDest = DestOffset;
    }

    return Status;
}


NDIS_STATUS
HWGetDestinationAddress(
    _In_  PNET_BUFFER  NetBuffer,
    _Out_writes_bytes_(NIC_MACADDR_SIZE) PUCHAR DestAddress)
/*++

Routine Description:

    Returns the destination address of a NET_BUFFER that is to be sent.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    NetBuffer                   The NB containing the frame that is being sent
    DestAddress                 On return, receives the frame's destination

Return Value:

    NDIS_STATUS_FAILURE         The frame is too short
    NDIS_STATUS_SUCCESS         Else

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NIC_FRAME_HEADER Header;
    ULONG cbHeader = sizeof(Header);

    Status = HWCopyBytesFromNetBuffer(NetBuffer, &cbHeader, &Header);
    if(Status == NDIS_STATUS_SUCCESS)
    {
        if (cbHeader < sizeof(Header))
        {
            NdisZeroMemory(DestAddress, NIC_MACADDR_SIZE);
            Status = NDIS_STATUS_FAILURE;
        }
        else
        {
            GET_DESTINATION_OF_FRAME(DestAddress, &Header);
        }
    }
	else
	{
		NdisZeroMemory(DestAddress, NIC_MACADDR_SIZE);
	}

    return Status;
}

BOOLEAN
HWIsFrameAcceptedByPacketFilter(
    _In_  PMP_ADAPTER  Adapter,
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress,
    _In_  ULONG        FrameType)
/*++

Routine Description:

    This routines checks to see whether the packet can be accepted
    for transmission based on the currently programmed filter type
    of the NIC and the mac address of the packet.

    With real adapter, this routine would be implemented in hardware.  However,
    since we don't have any hardware to do the matching for us, we'll do it in
    the driver.

Arguments:

    Adapter                     Our adapter that is receiving a frame
    FrameData                   The raw frame, starting at the frame header
    cbFrameData                 Number of bytes in the FrameData buffer

Return Value:

    TRUE if the frame is accepted by the packet filter, and should be indicated
    up to the higher levels of the stack.

    FALSE if the frame doesn't match the filter, and should just be dropped.

--*/
{
    BOOLEAN     result = FALSE;

    DEBUGP(MP_LOUD, "[%p] ---> HWIsFrameAcceptedByPacketFilter PacketFilter = 0x%08x, FrameType = 0x%08x\n",
            Adapter,
            Adapter->PacketFilter,
            FrameType);

    do
    {
        //
        // If the NIC is in promiscuous mode, we will accept anything
        // and everything.
        //
        if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
        {
            result = TRUE;
            break;
        }


        switch (FrameType)
        {
            case NDIS_PACKET_TYPE_BROADCAST:
                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
                {
                    //
                    // If it's a broadcast packet and broadcast is enabled,
                    // we can accept that.
                    //
                    result = TRUE;
                }
                break;


            case NDIS_PACKET_TYPE_MULTICAST:
                //
                // If it's a multicast packet and multicast is enabled,
                // we can accept that.
                //
                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
                {
                    result = TRUE;
                    break;
                }
                else if (Adapter->PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
                {
                    ULONG index;

                    //
                    // Check to see if the multicast address is in our list
                    //
                    ASSERT(Adapter->ulMCListSize <= NIC_MAX_MCAST_LIST);
                    for (index=0; index < Adapter->ulMCListSize && index < NIC_MAX_MCAST_LIST; index++)
                    {
                        if (NIC_ADDR_EQUAL(DestAddress, Adapter->MCList[index]))
                        {
                            result = TRUE;
                            break;
                        }
                    }
                }
                break;


            case NDIS_PACKET_TYPE_DIRECTED:

                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_DIRECTED)
                {
                    //
                    // This has to be a directed packet. If so, does packet dest
                    // address match with the mac address of the NIC.
                    //
                    if (NIC_ADDR_EQUAL(DestAddress, Adapter->CurrentAddress))
                    {
                        result = TRUE;
                        break;
                    }
                }

                break;
        }

    } while(FALSE);


    DEBUGP(MP_LOUD, "[%p] <--- HWIsFrameAcceptedByPacketFilter Result = %u\n", Adapter, result);
    return result;
}


NDIS_MEDIA_CONNECT_STATE
HWGetMediaConnectStatus(
    _In_  PMP_ADAPTER Adapter)
/*++

Routine Description:

    This routine will query the hardware and return
    the media status.

Arguments:

    Adapter                     Our Adapter

Return Value:

    NdisMediaStateDisconnected or
    NdisMediaStateConnected

--*/
{
    if (MP_TEST_FLAG(Adapter, fMP_DISCONNECTED))
    {
        return MediaConnectStateDisconnected;
    }
    else
    {
        return MediaConnectStateConnected;
    }
}

VOID
HWProgramDmaForSend(
    _In_  PMP_ADAPTER   Adapter,
    _In_  PTCB          Tcb
)
/*++

Routine Description:

    Program the hardware to read the data payload from the NET_BUFFER's MDL
    and queue it for transmission.  When the hardware has finished reading the
    MDL, it will fire an interrupt to indicate that it no longer needs the MDL
    anymore.

    Our hardware, of course, doesn't have any DMA, so it just copies the data
    to a FRAME structure and transmits that.


    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Our adapter that will send a frame
    Tcb                         The TCB that tracks the transmit status
    NetBuffer                   Contains the data to send
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{
	PNET_BUFFER NetBuffer = NULL;
	PMDL CurrentMdl = NULL;
	BOOLEAN FirstMdl = FALSE;

    DEBUGP(MP_TRACE, "[%p] ---> HWProgramDmaForSend.\n", Adapter);

	NetBuffer = Tcb->NetBuffer;	
	
	CurrentMdl = NET_BUFFER_CURRENT_MDL(NetBuffer);
	FirstMdl = TRUE;
	Adapter->nTxFrame ++;
	
	while(CurrentMdl)
	{
		PUCHAR SrcMemory = MmGetSystemAddressForMdlSafe(CurrentMdl, LowPagePriority);
		ULONG Length = MmGetMdlByteCount(CurrentMdl);
		
		if(FirstMdl)
		{	
			ULONG MdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer);
			SrcMemory += MdlOffset;
			Length -= MdlOffset;
			FirstMdl = FALSE;
		}
		//Dbg_Dump_Data(TRUE, Adapter->nTxFrame, SrcMemory, Length);
		if(Tcb->BytesSent + Length > Tcb->BufLen)
		{
			DEBUGP(MP_TRACE, "[%p] HWProgramDmaForSend: Data len %d in buffer, more %d bytes needs \n", Adapter, Tcb->BytesSent, Length);
		}
		else
		{
			NdisMoveMemory((PUCHAR)(Tcb->DataBuffer + Tcb->BytesSent), SrcMemory, Length);
		}
		
		Tcb->BytesSent += Length;
		
		CurrentMdl = NDIS_MDL_LINKAGE(CurrentMdl);
	}
	
	HW_DMA_Set_Buffer(Tcb->DmaDesc, Tcb->PhyAddress, Tcb->BytesSent);

	Adapter->FirstFreeTxDMAIndex = INC_TX_DMA_INDEX(Adapter->FirstFreeTxDMAIndex);

	// To HW DMA Controller
	// Set First and Last Flags
	HW_DMA_Set_Tx_First(Tcb->DmaDesc);
	HW_DMA_Set_Tx_Last(Tcb->DmaDesc);

	// Set DMA Owner bit
	HW_DMA_Set_Own(Tcb->DmaDesc);

    DEBUGP(MP_TRACE, "[%p] <--- HWProgramDmaForSend  NB: 0x%p, %d bytes \n", Adapter, NetBuffer, Tcb->BytesSent);
	NICAddDbgLog(Adapter, 'TDma', 
		(Adapter->nTxFrame | 0x4E000000), 
		(Adapter->FirstFreeTxDMAIndex | 0x69460000), 
		(Tcb->BytesSent | 0x4C000000));

}

NDIS_STATUS
HWReceiveDma(
    _In_  PMP_ADAPTER  Adapter,
    _In_  PRCB         Rcb)
/*++

Routine Description:

    Simulate the hardware deciding to receive a FRAME into one of its RCBs. In VMQ enabled scenarios, it will 
    find the matching queue and if matched retrieve the shared memory for the queue for the NBL. Otherwise, it 
    uses the existing Frame for the NBL.

Arguments:

    Adapter                     Pointer to our adapter
    Rcb                         The RCB that tracks this receive operation
    Desc                       The FRAME that to receive

Return Value:

    NDIS_STATUS

--*/
{
	UNREFERENCED_PARAMETER(Adapter);
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	ULONG	    DmaStatus = 0;
	ULONG		SkipLen = 0;
	PDMA_DESC	Desc = Rcb->DmaDesc;
    PNET_BUFFER NetBuffer = NET_BUFFER_LIST_FIRST_NB(Rcb->Nbl);
	
	
	DEBUGP(MP_TRACE, "[%p] ---> HWReceiveDma. \n", Adapter);

    do
    {
		if(HW_DMA_Get_Owner_Bit(Desc) & DMA_HW_OWN)
		{
			Status = NDIS_STATUS_PENDING;
			break;
		}

		DmaStatus = HW_DMA_Get_Status(Desc);
		Adapter->nRxFrame ++;		
		Rcb->RecvLen = HW_DMA_Get_Rx_Len(Desc);

		NICAddDbgLog(Adapter, 'RDma', 
			(Adapter->nRxFrame | 0x4E000000), 
			(Rcb->RecvLen| 0x4C000000), DmaStatus);

		if(Rcb->RecvLen < HW_MIN_FRAME_SIZE)
		{
			DEBUGP(MP_WARNING, "[%p] HWReceiveDma Frame too short %d. \n", Adapter, Rcb->RecvLen);
			Rcb->RecvLen = 0;
			Status = NDIS_STATUS_DATA_NOT_ACCEPTED;
			break;
		}

		if(DmaStatus  & DMA_RX_ERROR_BITS)
		{
			DEBUGP(MP_ERROR, "[%p] HWReceiveDma DMA Error Status 0x%x, Lentgth %d. \n", Adapter, DmaStatus, Rcb->RecvLen);
			//Status = NDIS_STATUS_DATA_NOT_ACCEPTED;
			//break;
		}

		if((Rcb->DataBuffer[14] == 0) && (Rcb->DataBuffer[15] == 1))
		{
			SkipLen = 2;
			DEBUGP(MP_WARNING, "[%p] HWReceiveDma ARP Packet, length %d, skip %d. \n", Adapter, Rcb->RecvLen, SkipLen);				
		}

		if(SkipLen)
		{
			NdisMoveMemory((PUCHAR)(Rcb->DataBuffer + SkipLen), Rcb->DataBuffer, Rcb->RecvLen);
		}

		Rcb->RecvLen -= 0;//HW_FCS_SIZE;		// Remove FCS

        NET_BUFFER_FIRST_MDL(NetBuffer) = Rcb->Mdl;
        NET_BUFFER_DATA_LENGTH(NetBuffer) = Rcb->RecvLen;
        NET_BUFFER_DATA_OFFSET(NetBuffer) = NIC_RECV_BUFFER_SKIP_SIZE + SkipLen;
        NET_BUFFER_CURRENT_MDL(NetBuffer) = NET_BUFFER_FIRST_MDL(NetBuffer);
        NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer) = NIC_RECV_BUFFER_SKIP_SIZE + SkipLen;    
    }
    while(FALSE);
     
    DEBUGP(MP_TRACE, "[%p] <--- HWReceiveDma Status 0x%08x, length %d \n", Adapter, Status, Rcb->RecvLen);
    return Status;

}

NDIS_STATUS
HWStartStopTxRx(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      Start)
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	PHYSICAL_ADDRESS PhyAddress;

	// Start/Stop Tx
	PhyAddress = MmGetPhysicalAddress(Adapter->TxDmaDescPool);
	HW_MAC_Start_Stop_DMA(&Adapter->PhyAdapter->Mac, Start, TRUE, PhyAddress.LowPart);

	// Start/Stop Rx
	PhyAddress = MmGetPhysicalAddress(Adapter->RxDmaDescPool);
	HW_MAC_Start_Stop_DMA(&Adapter->PhyAdapter->Mac, Start, FALSE, PhyAddress.LowPart);

	return Status;
}

VOID Dbg_Dump_Data(BOOLEAN Tx, ULONG Frame, PUCHAR Data, ULONG Length)
{
	ULONG index = 0;
	PUCHAR Buffer = (PUCHAR)Data;

	if(Tx)
	{
		DbgPrintEx(0,0,"Tx Frame %d\n", Frame);
	}
	else
	{
		DbgPrintEx(0,0,"Rx Frame %d\n", Frame);
	}

	while(index < Length)
	{
		DbgPrintEx(0,0,"%02x ", *(Buffer + index));
		index ++;
		if((index % 8) == 0)
		{
			DbgPrintEx(0,0,"  ");
		}
		if((index % 16) == 0)
		{
			DbgPrintEx(0,0,"\n");
		}	
	}
	DbgPrintEx(0,0,"\n");
}
 
