/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    DataPath.C

Abstract:

    This module implements the data path of the netvmini miniport.

    In order to excercise the data path of this driver,
    you should install more than one instance of the miniport. If there
    is only one instance installed, the driver throws the send packet on
    the floor and completes the send successfully. If there are more
    instances present, it indicates the incoming send packet to the other
    instances. For example, if there 3 instances: A, B, & C installed.
    Frames sent on instance A would be received on B & C; frames
    sent on B would be received on C, & A; and frames sent on C
    would be received on A & B.

    This sample miniport goes to some extra lengths so that the data path's
    design resembles the design of a real hardware miniport's data path.  For
    example, this sample has both send and receive queues, even though all the
    miniports on the simulated network are on the same computer (and thus could
    take some shortcuts when passing data buffers back and forth).



--*/

#include "Ethmini.h"
#include "datapath.tmh"

static
VOID
TXQueueNetBufferForSend(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER       NetBuffer);

static
VOID
TXTransmitQueuedSends(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      fAtDispatch);

_Must_inspect_result_
static
PTCB
TXGetNextTcbToSend(
    _In_  PMP_ADAPTER      Adapter);

#pragma NDIS_PAGEABLE_FUNCTION(NICStartTheDatapath)
#pragma NDIS_PAGEABLE_FUNCTION(NICStopTheDatapath)

VOID
MPSendNetBufferLists(
    _In_  NDIS_HANDLE             MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST        NetBufferLists,
    _In_  NDIS_PORT_NUMBER        PortNumber,
    _In_  ULONG                   SendFlags)
/*++

Routine Description:

    Send Packet Array handler. Called by NDIS whenever a protocol
    bound to our miniport sends one or more packets.

    The input packet descriptor pointers have been ordered according
    to the order in which the packets should be sent over the network
    by the protocol driver that set up the packet array. The NDIS
    library preserves the protocol-determined ordering when it submits
    each packet array to MiniportSendPackets

    As a deserialized driver, we are responsible for holding incoming send
    packets in our internal queue until they can be transmitted over the
    network and for preserving the protocol-determined ordering of packet
    descriptors incoming to its MiniportSendPackets function.
    A deserialized miniport driver must complete each incoming send packet
    with NdisMSendComplete, and it cannot call NdisMSendResourcesAvailable.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              Head of a list of NBLs to send
    PortNumber                  A miniport adapter port.  Default is 0.
    SendFlags                   Additional flags for the send operation

Return Value:

    None.  Write status directly into each NBL with the NET_BUFFER_LIST_STATUS
    macro.

--*/
{
    PMP_ADAPTER       Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    PNET_BUFFER_LIST  Nbl;
    PNET_BUFFER_LIST  NextNbl = NULL;
    BOOLEAN           fAtDispatch = (SendFlags & NDIS_SEND_FLAGS_DISPATCH_LEVEL) ? TRUE:FALSE;
    NDIS_STATUS       Status = NDIS_STATUS_SUCCESS;
    ULONG             NumNbls=0;

    DEBUGP(MP_TRACE, "[%p] ---> MPSendNetBufferLists\n", Adapter);

    UNREFERENCED_PARAMETER(PortNumber);
    UNREFERENCED_PARAMETER(SendFlags);
    ASSERT(PortNumber == 0); // Only the default port is supported


    //
    // Each NET_BUFFER_LIST has a list of NET_BUFFERs.
    // Loop over all the NET_BUFFER_LISTs, sending each NET_BUFFER.
    //
    for (
        Nbl = NetBufferLists;
        Nbl!= NULL;
        Nbl = NextNbl, ++NumNbls)
    {
    	PNET_BUFFER NetBuffer;
        NextNbl = NET_BUFFER_LIST_NEXT_NBL(Nbl);

        //
        // Unlink the NBL and prepare our bookkeeping.
        //
        // We use a reference count to make sure that we don't send complete
        // the NBL until we're done reading each NB on the NBL.
        //
        NET_BUFFER_LIST_NEXT_NBL(Nbl) = NULL;
		
        SEND_REF_FROM_NBL(Nbl) = 0;

        Status = TXNblReference(Adapter, Nbl);

        if(Status == NDIS_STATUS_SUCCESS)
        {
            NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_SUCCESS;

            //
            // Queue each NB for transmission.
            //
            for (
                NetBuffer = NET_BUFFER_LIST_FIRST_NB(Nbl);
                NetBuffer != NULL;
                NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
            {
                NBL_FROM_SEND_NB(NetBuffer) = Nbl;
                TXQueueNetBufferForSend(Adapter, NetBuffer);
            }

            TXNblRelease(Adapter, Nbl, fAtDispatch);
        }
        if(Status != NDIS_STATUS_SUCCESS)
        {        	
            //
            // We can't send this NBL now.  Indicate failure.
            //
            if (MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_RESET_IN_PROGRESS;
            }
            else if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS|fMP_ADAPTER_PAUSED))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_PAUSED;
            }
            else if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LOW_POWER))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_LOW_POWER_STATE;
            }
            else
            {
                NET_BUFFER_LIST_STATUS(Nbl) = Status;
            }

			TXNblRelease(Adapter, Nbl, fAtDispatch);

            NdisMSendNetBufferListsComplete(
                    Adapter->AdapterHandle,
                    Nbl,
                    fAtDispatch ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL:0);

            continue;
        }
    }

    DEBUGP(MP_TRACE, "[%p] MPSendNetBufferLists: %i NBLs processed.\n", Adapter, NumNbls);

	NICAddDbgLog(Adapter, 'SNbl', (NumNbls | 0x6C4E0000), 0, Status);

    //
    // Now actually go send each of the queued NBs.
    //
    TXTransmitQueuedSends(Adapter, fAtDispatch);

    DEBUGP(MP_TRACE, "[%p] <--- MPSendNetBufferLists\n", Adapter);
}

VOID
TXQueueNetBufferForSend(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER       NetBuffer)
/*++

Routine Description:

    This routine inserts the NET_BUFFER into the SendWaitList, then calls
    TXTransmitQueuedSends to start sending data from the list.

    We use this indirect queue to send data because the miniport should try to
    send frames in the order in which the protocol gave them.  If we just sent
    the NET_BUFFER immediately, then it would be out-of-order with any data on
    the SendWaitList.


    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Adapter that is transmitting this NB
    NetBuffer                   NB to be transfered

Return Value:

    None.

--*/
{
    NDIS_STATUS       Status;
    //UCHAR             DestAddress[NIC_MACADDR_SIZE];

    DEBUGP(MP_TRACE, "[%p] ---> TXQueueNetBufferForSend, NB= 0x%p\n", Adapter, NetBuffer);

    do
    {
        //
        // First, do a sanity check on the frame data.
        //
        //Status = HWGetDestinationAddress(NetBuffer, DestAddress);
        //if (Status != NDIS_STATUS_SUCCESS)
        //{
        //    NET_BUFFER_LIST_STATUS(NBL_FROM_SEND_NB(NetBuffer)) = NDIS_STATUS_INVALID_DATA;
        //    break;
        //}

        //
        // Stash away the frame type.  We'll use that later, when updating
        // our send statistics (since we don't have NIC hardware to compute the
        // send statistics for us).
        //
        FRAME_TYPE_FROM_SEND_NB(NetBuffer) = NDIS_PACKET_TYPE_DIRECTED;//NICGetFrameTypeFromDestination(DestAddress);

        //
        // Pin the original NBL with a reference, so it isn't completed until
        // we're done with its NB.
        //
        Status = TXNblReference(Adapter, NBL_FROM_SEND_NB(NetBuffer));
        if(Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Insert the NB into the queue.  The caller will flush the queue when
            // it's done adding items to the queue.
            //
            NdisInterlockedInsertTailList(
                    &Adapter->SendWaitList,
                    SEND_WAIT_LIST_FROM_NB(NetBuffer),
                    &Adapter->SendWaitListLock);
        }

    } while (FALSE);


    DEBUGP(MP_TRACE, "[%p] <--- TXQueueNetBufferForSend\n", Adapter);
}

VOID
#pragma prefast(suppress: 28167, "PREfast does not recognize IRQL is conditionally raised and lowered")
TXTransmitQueuedSends(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      fAtDispatch)
/*++

Routine Description:

    This routine sends as many frames from the SendWaitList as it can.

    If there are not enough resources to send immediately, this function stops
    and leaves the remaining frames on the SendWaitList, to be sent once there
    are enough resources.


    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    Adapter                     Our adapter
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{
    KIRQL OldIrql = PASSIVE_LEVEL;
	ULONG NumNbsSent = 0;

    DEBUGP(MP_TRACE,
           "[%p] ---> TXTransmitQueuedSends\n",
           Adapter);

    //
    // This guard ensures that only one CPU is running this function at a time.
    // We check this so that items from the SendWaitList get sent to the
    // receiving adapters in the same order that they were queued.
    //
    // You could remove this guard and everything will still work ok, but some
    // frames might be delivered out-of-order.
    //
    // Generally, this mechanism wouldn't be applicable to real hardware, since
    // the hardware would have its own mechanism to ensure sends are transmitted
    // in the correct order.
    //
    if (!fAtDispatch)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }

    if (KeTryToAcquireSpinLockAtDpcLevel(&Adapter->SendPathSpinLock))
    {
		PLIST_ENTRY pQueuedSend = NULL;
    	while(TRUE)
    	{
    		PLIST_ENTRY pTcbEntry = NULL;
            PTCB Tcb = NULL;
            PNET_BUFFER NetBuffer = NULL;
			
    		//
            // Get the next available TCB.
            //
            pTcbEntry = NdisInterlockedRemoveHeadList(
                    &Adapter->FreeTcbList,
                    &Adapter->FreeTcbListLock);
            if (!pTcbEntry)
            {
                //
                // The adapter can't handle any more simultaneous transmit
                // operations.  Keep any remaining sends in the SendWaitList and
                // we'll come back later when there are TCBs available.
                //
 
                break;
            }
			Tcb = CONTAINING_RECORD(pTcbEntry, TCB, TcbLink);
			
			//
            // Get the next NB that needs sending.
            //
            pQueuedSend = NdisInterlockedRemoveHeadList(
                    &Adapter->SendWaitList,
                    &Adapter->SendWaitListLock);
            if (!pQueuedSend)
            {
                //
                // There's nothing left that needs sending.  We're all done.
                //
                NdisInterlockedInsertHeadList(
                        &Adapter->FreeTcbList,
                        &Tcb->TcbLink,
                        &Adapter->FreeTcbListLock);
                break;
            }

			NetBuffer = NB_FROM_SEND_WAIT_LIST(pQueuedSend);
			
			Tcb->NetBuffer = NetBuffer;
			Tcb->FrameType = FRAME_TYPE_FROM_SEND_NB(NetBuffer);
			
			HWProgramDmaForSend(Adapter, Tcb);

			Adapter->TxDmaUsed += 1;
			NumNbsSent++;

			NdisInterlockedInsertTailList(
                &Adapter->BusyTcbList,
                    &Tcb->TcbLink,
                    &Adapter->BusyTcbListLock);
    	}
        KeReleaseSpinLock(&Adapter->SendPathSpinLock, DISPATCH_LEVEL);
    }

	if(NumNbsSent)
	{
		HW_MAC_Start_DMA_Transfer(&Adapter->PhyAdapter->Mac, TRUE);
	}

    if (!fAtDispatch)
    {
        KeLowerIrql(OldIrql);
    }

    DEBUGP(MP_TRACE, "[%p] TXTransmitQueuedSends %i Frames transmitted, TxBusy %d, TxFree %d.\n", Adapter, NumNbsSent, Adapter->FirstBusyTxDMAIndex, Adapter->FirstFreeTxDMAIndex);
	NICAddDbgLog(Adapter, 'QSNb', (NumNbsSent  | 0x4E000000), 
		(Adapter->FirstBusyTxDMAIndex | 0x69420000), 
		(Adapter->FirstFreeTxDMAIndex | 0x69460000));

    DEBUGP(MP_TRACE, "[%p] <-- TXTransmitQueuedSends\n", Adapter);
}





_IRQL_requires_(DISPATCH_LEVEL)
VOID
TXSendComplete(
    _In_ PMP_ADAPTER Adapter)
/*++

Routine Description:

    This routine completes pending sends for the given adapter.

    Each busy TCB is popped from the BusyTcbList and its corresponding NB is
    released.  If there was an error sending the frame, the NB's NBL's status
    is updated.

--*/
{
	PTCB Tcb;
	ULONG	TcbStatus;
	BOOLEAN	DmaListEmpty = TRUE;

    DEBUGP(MP_TRACE, "[%p] ---> TXSendComplete.\n", Adapter);

    while((Tcb = TXGetNextTcbToSend(Adapter)) != NULL)
    {
		if(HW_DMA_Get_Owner_Bit(Tcb->DmaDesc)
			& DMA_HW_OWN)
		{
			NdisInterlockedInsertHeadList(
                &Adapter->BusyTcbList,
                    &Tcb->TcbLink,
                    &Adapter->BusyTcbListLock);
			DmaListEmpty = FALSE;
			break;
		}
        else
        {
        	TcbStatus = HW_DMA_Get_Status(Tcb->DmaDesc);

			// Release DMA Descriptor in this TCB
			DEBUGP(MP_TRACE, "[%p] TXSendComplete, DMA Status 0x%x.\n", Adapter, TcbStatus);
			if(TcbStatus & DMA_TX_ERROR_BITS)
			{
				DbgPrintEx(0,0, "[%p] TXSendComplete DMA index %d Status 0x%x. \n", Adapter, Adapter->FirstBusyTxDMAIndex, TcbStatus);
			}
			HW_DMA_Desc_Reuse(Tcb->DmaDesc);
			
			Adapter->TxDmaUsed -= 1;
			Adapter->FirstBusyTxDMAIndex = INC_TX_DMA_INDEX(Adapter->FirstBusyTxDMAIndex);
			DEBUGP(MP_TRACE, "[%p] TXSendComplete, Release Dma.\n", Adapter);

            switch (Tcb->FrameType)
            {
                case NDIS_PACKET_TYPE_BROADCAST:
                    Adapter->FramesTxBroadcast += 1;
                    Adapter->BytesTxBroadcast += Tcb->BytesSent;
                    break;

                case NDIS_PACKET_TYPE_MULTICAST:
                    Adapter->FramesTxMulticast += 1;
                    Adapter->BytesTxMulticast += Tcb->BytesSent;
                    break;

                case NDIS_PACKET_TYPE_DIRECTED:
                default:
                    Adapter->FramesTxDirected += 1;
                    Adapter->BytesTxDirected += Tcb->BytesSent;
            }
			ReturnTCB(Adapter, Tcb);
        }   
    }

	DEBUGP(MP_TRACE, "[%p] TXSendComplete, TxBusy %d, TxFree %d.\n", Adapter, Adapter->FirstBusyTxDMAIndex, Adapter->FirstFreeTxDMAIndex);
	NICAddDbgLog(Adapter, 'TDPC', 
		(Adapter->FirstBusyTxDMAIndex | 0x69420000), 
		(Adapter->FirstFreeTxDMAIndex | 0x69460000), 
		(DmaListEmpty | 0x66450000));

	// Start DMA transmit and program DMA if there is any
	if(!DmaListEmpty)
	{
		HW_MAC_Start_DMA_Transfer(&Adapter->PhyAdapter->Mac, TRUE);
	}
	TXTransmitQueuedSends(Adapter, TRUE);

    DEBUGP(MP_TRACE, "[%p] <--- TXSendComplete.\n", Adapter);
}


NDIS_STATUS
TXNblReference(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER_LIST  NetBufferList)
/*++

Routine Description:

    Adds a reference on a NBL that is being transmitted.
    The NBL won't be returned to the protocol until the last reference is
    released.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter
    NetBufferList               The NBL to reference

Return Value:

    NDIS_STATUS_SUCCESS if reference was acquired succesfully.
    NDIS_STATUS_ADAPTER_NOT_READY if the adapter state is such that we should not acquire new references to resources

--*/
{
	UNREFERENCED_PARAMETER(NetBufferList);
    NdisInterlockedIncrement(&Adapter->nBusySend);

    //
    // Make sure the increment happens before ready state check
    //
    KeMemoryBarrier();

    //
    // If the adapter is not ready, undo the reference and fail the call
    //
    if(!MP_IS_READY(Adapter))
    {
        InterlockedDecrement(&Adapter->nBusySend);
        DEBUGP(MP_LOUD, "[%p] Could not acquire transmit reference, the adapter is not ready.\n", Adapter);
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }

    NdisInterlockedIncrement(&SEND_REF_FROM_NBL(NetBufferList));
    return NDIS_STATUS_SUCCESS;

}


VOID
TXNblRelease(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER_LIST  NetBufferList,
    _In_  BOOLEAN           fAtDispatch)
/*++

Routine Description:

    Releases a reference on a NBL that is being transmitted.
    If the last reference is released, the NBL is returned to the protocol.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter
    NetBufferList               The NBL to release
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{

    if (0 == NdisInterlockedDecrement(&SEND_REF_FROM_NBL(NetBufferList)))
    {
        DEBUGP(MP_TRACE, "[%p] Send NBL %p complete.\n", Adapter, NetBufferList);

        NET_BUFFER_LIST_NEXT_NBL(NetBufferList) = NULL;

        NdisMSendNetBufferListsComplete(
                Adapter->AdapterHandle,
                NetBufferList,
                fAtDispatch ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL:0);
    }
    else
    {
        DEBUGP(MP_TRACE, "[%p] Send NBL %p not complete. RefCount: %i.\n", Adapter, NetBufferList, SEND_REF_FROM_NBL(NetBufferList));
    }

    NdisInterlockedDecrement(&Adapter->nBusySend);
}

NDIS_STATUS
TxNetBufferCheck(_In_  PNET_BUFFER  NetBuffer)
{
	NDIS_STATUS       Status = NDIS_STATUS_SUCCESS;

	// First check the buffer data size
	if((NET_BUFFER_DATA_LENGTH(NetBuffer) < NIC_ETHER_SIZE) 
		|| (NET_BUFFER_DATA_LENGTH(NetBuffer) > HW_MAX_FRAME_SIZE))
	{
		goto FAIL;
	}

	// Other checks

	goto Exit;
	
FAIL:
	Status = NDIS_STATUS_INVALID_DATA;
Exit:
	return Status;
}

_Must_inspect_result_
PTCB
TXGetNextTcbToSend(
    _In_  PMP_ADAPTER      Adapter)
/*++

Routine Description:

    Returns the next TCB queued on the send list, or NULL if the list was empty.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    NULL if there was no TCB queued.
    Else, a pointer to the TCB that was popped off the top of the BusyTcbList.

--*/
{
    PTCB Tcb;
    PLIST_ENTRY pTcbEntry = NdisInterlockedRemoveHeadList(
            &Adapter->BusyTcbList,
            &Adapter->BusyTcbListLock);

    if (! pTcbEntry)
    {
        // End of list -- no more items to receive.
        return NULL;
    }

    Tcb = CONTAINING_RECORD(pTcbEntry, TCB, TcbLink);

    ASSERT(Tcb);
    ASSERT(Tcb->NetBuffer);

    return Tcb;
}


VOID
TXFlushSendQueue(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_STATUS  CompleteStatus)
/*++

Routine Description:

    This routine is called by the Halt or Reset handler to fail all
    the queued up Send NBLs because the device is either gone, being
    stopped for resource rebalance, or reset.

Arguments:

    Adapter                     Pointer to our adapter
    CompleteStatus              The status code with which to complete each NBL

Return Value:

    None.

--*/
{
    PTCB Tcb;

    DEBUGP(MP_TRACE, "[%p] ---> TXFlushSendQueue Status = 0x%08x\n", Adapter, CompleteStatus);


    //
    // First, free anything queued in the driver.
    //

    while (TRUE)
    {
        PLIST_ENTRY pEntry;
        PNET_BUFFER_LIST NetBufferList;

        pEntry = NdisInterlockedRemoveHeadList(
                &Adapter->SendWaitList,
                &Adapter->SendWaitListLock);

        if (!pEntry)
        {
            // End of list -- nothing left to free.
            break;
        }

        NetBufferList = NBL_FROM_LIST_ENTRY(pEntry);

        DEBUGP(MP_TRACE, "[%p] Dropping Send NB: 0x%p.\n", Adapter, NetBufferList);

        NET_BUFFER_LIST_STATUS(NetBufferList) = CompleteStatus;
        TXNblRelease(Adapter, NetBufferList, FALSE);
    }


    //
    // Next, cancel anything queued in the hardware.
    //

    while (NULL != (Tcb = TXGetNextTcbToSend(Adapter)))
    {
        NET_BUFFER_LIST_STATUS(NBL_FROM_SEND_NB(Tcb->NetBuffer)) = CompleteStatus;
        ReturnTCB(Adapter, Tcb);
    }

    DEBUGP(MP_TRACE, "[%p] <--- TXFlushSendQueue\n", Adapter);
}


BOOLEAN
RXReceiveIndicate(
    _In_ PMP_ADAPTER Adapter,
    _In_ ULONG maxNblsToIndicate)
/*++

Routine Description:

    This function performs the receive indications for the specified RECEIVE_DPC structure.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter             Pointer to our adapter
    maxNblsToIndicate          

Return Value:

    None.

--*/

{

    ULONG NumNblsReceived = 0;
	ULONG DmaReleased = 0;
    PNET_BUFFER_LIST FirstNbl = NULL, LastNbl = NULL;
	BOOLEAN moreNblsPending = FALSE;	
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	PRCB Rcb = NULL;

    DEBUGP(MP_TRACE, "[%p] ---> RXReceiveIndicate. maxNblsToIndicate: %i\n", Adapter, maxNblsToIndicate);

	FirstNbl = LastNbl = NULL;

	//
	// Collect pending NBLs, indicate up to MaxNblCountPerIndicate per receive block
	//
	while(NumNblsReceived < maxNblsToIndicate)
	{
		Rcb = &((PRCB)Adapter->RcbMemoryBlock)[Adapter->RxDmaDescIndex];

		if(Rcb->RecvLen != 0)
		{
			DEBUGP(MP_WARNING, "[%p]: RX No Free Buffers.\n", Adapter);
			break;
		}

		Status = HWReceiveDma(Adapter, Rcb);
		if(Status == NDIS_STATUS_PENDING)
		{
			// DMA buffer not ready
			break;
		}
		
		DmaReleased ++;
		if(Status == NDIS_STATUS_DATA_NOT_ACCEPTED)
		{
			// Data Error, Reuse RCB and DMA buffer
			ReturnRCB(Adapter, Rcb);
			Adapter->RxDmaDescIndex = INC_RX_DMA_INDEX(Adapter->RxDmaDescIndex);;
			continue;
		}

		//
		// The recv NBL's data was filled out by the hardware.	Now just update
		// its bookkeeping.
		//
		NET_BUFFER_LIST_STATUS(Rcb->Nbl) = NDIS_STATUS_SUCCESS;
		Rcb->Nbl->SourceHandle = Adapter->AdapterHandle;
		Adapter->RxDmaDescIndex = INC_RX_DMA_INDEX(Adapter->RxDmaDescIndex);
		NumNblsReceived++;

		//
		// Add this NBL to the chain of NBLs to indicate up.
		//
		if (!FirstNbl)
		{
			LastNbl = FirstNbl = Rcb->Nbl;
		}
		else
		{
			NET_BUFFER_LIST_NEXT_NBL(LastNbl) = Rcb->Nbl;
			LastNbl = Rcb->Nbl;
		}
	}

	//
	// Indicate NBLs
	//
	if (FirstNbl)
	{
		DEBUGP(MP_TRACE, "[%p]: %i frames indicated.\n", Adapter, NumNblsReceived);

		NET_BUFFER_LIST_NEXT_NBL(LastNbl) = NULL;

		//
		// Indicate up the NBLs.
		//
		// The NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL allows a perf optimization:
		// NDIS doesn't have to check and raise the current IRQL, since we
		// promise that the current IRQL is exactly DISPATCH_LEVEL already.
		//
		NdisMIndicateReceiveNetBufferLists(
				Adapter->AdapterHandle,
				FirstNbl,
				0,	// default port
				NumNblsReceived,
				NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL
				// | NDIS_RECEIVE_FLAGS_PERFECT_FILTERED
				);

		Adapter->nBusyInd += NumNblsReceived;
			
		if(NumNblsReceived == maxNblsToIndicate)
		{			
			Rcb = &((PRCB)Adapter->RcbMemoryBlock)[Adapter->RxDmaDescIndex];
			if(HW_DMA_Get_Owner_Bit(Rcb->DmaDesc) != DMA_HW_OWN)
			{
				moreNblsPending = TRUE;
			}
		}
	}

    DEBUGP(MP_TRACE, "[%p] <--- RXReceiveIndicate. Dma Finished %d, NBL indicated %d, Rx %d Frames, RxDmaDescIndex %d, moreNblsPending: %i\n", 
					Adapter, DmaReleased, NumNblsReceived, Adapter->nRxFrame,Adapter->RxDmaDescIndex, moreNblsPending);
	NICAddDbgLog(Adapter, 'RDPC', 
		(DmaReleased | 0x724E0000), 
		(NumNblsReceived | 0x6C4E0000), 
		(Adapter->RxDmaDescIndex | 0x69520000));

	return moreNblsPending;
}


VOID
MPReturnNetBufferLists(
    _In_  NDIS_HANDLE       MiniportAdapterContext,
    _In_  PNET_BUFFER_LIST  NetBufferLists,
    _In_  ULONG             ReturnFlags)
/*++

Routine Description:

    NDIS Miniport entry point called whenever protocols are done with one or
    NBLs that we indicated up with NdisMIndicateReceiveNetBufferLists.

    Note that the list of NBLs may be chained together from multiple separate
    lists that were indicated up individually.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              NBLs being returned
    ReturnFlags                 May contain the NDIS_RETURN_FLAGS_DISPATCH_LEVEL
                                flag, which if is set, indicates we can get a
                                small perf win by not checking or raising the
                                IRQL

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
	ULONG		NumNblsReturned = 0;
    UNREFERENCED_PARAMETER(ReturnFlags);

    DEBUGP(MP_TRACE, "[%p] ---> MPReturnNetBufferLists\n", Adapter);

    while (NetBufferLists)
    {
        PRCB Rcb = RCB_FROM_NBL(NetBufferLists);
        ReturnRCB(Adapter, Rcb);
        NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(NetBufferLists);
		NumNblsReturned++;
    }
	Adapter->nBusyInd -= NumNblsReturned;

    DEBUGP(MP_TRACE, "[%p] <--- MPReturnNetBufferLists\n", Adapter);
}

VOID
MPCancelSend(
    _In_  NDIS_HANDLE     MiniportAdapterContext,
    _In_  PVOID           CancelId)
/*++

Routine Description:

    MiniportCancelSend cancels the transmission of all NET_BUFFER_LISTs that
    are marked with a specified cancellation identifier. Miniport drivers
    that queue send packets for more than one second should export this
    handler. When a protocol driver or intermediate driver calls the
    NdisCancelSendNetBufferLists function, NDIS calls the MiniportCancelSend
    function of the appropriate lower-level driver (miniport driver or
    intermediate driver) on the binding.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    CancelId                    All the packets with this Id should be cancelled

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(CancelId);


    DEBUGP(MP_TRACE, "[%p] ---> MPCancelSend\n", Adapter);

    //
    // This miniport completes its sends quickly, so it isn't strictly
    // neccessary to implement MiniportCancelSend.
    //
    // If we did implement it, we'd have to walk the Adapter->SendWaitList
    // and look for any NB that points to a NBL where the CancelId matches
    // NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(Nbl).  For any NB that so matches,
    // we'd remove the NB from the SendWaitList and set the NBL's status to
    // NDIS_STATUS_SEND_ABORTED, then complete the NBL.
    //

    DEBUGP(MP_TRACE, "[%p] <--- MPCancelSend\n", Adapter);
}


VOID
NICStartTheDatapath(
    _In_  PMP_ADAPTER  Adapter)
/*++

Routine Description:

    This function enables sends and receives on the data path.  It is the
    reciprocal of NICStopTheDatapath.

    Runs at IRQL == PASSIVE_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    None.

--*/
{
    PAGED_CODE();
	
	PHYSICAL_ADDRESS 	PhyAddress;
	ULONG				index;

	DEBUGP(MP_TRACE, "[%p] ---> NICStartTheDatapath\n", Adapter);
	
    MPAttachAdapter(Adapter);

	// Reset DMA Descriptor buffers
	NdisZeroMemory(Adapter->TxDmaDescPool, sizeof(DMA_DESC) * NIC_MAX_BUSY_SENDS);
	PhyAddress = MmGetPhysicalAddress(Adapter->TxDmaDescPool);
	HW_DMA_Init_Desc_Chain(Adapter->TxDmaDescPool,PhyAddress.LowPart, NIC_MAX_BUSY_SENDS);
	Adapter->FirstBusyTxDMAIndex = 0;
	Adapter->FirstFreeTxDMAIndex = 0;
	Adapter->TxDmaUsed = 0;
	// ReInitialize the list
	NdisInitializeListHead(&Adapter->FreeTcbList);
	for (index = 0; index < NIC_MAX_BUSY_SENDS; index++)
    {		
        PTCB Tcb = &((PTCB)Adapter->TcbMemoryBlock)[index];
        NdisInterlockedInsertTailList(
                &Adapter->FreeTcbList,
                &Tcb->TcbLink,
                &Adapter->FreeTcbListLock);
    }

	NdisZeroMemory(Adapter->RxDmaDescPool, sizeof(DMA_DESC) * NIC_MAX_BUSY_RECVS);
	PhyAddress = MmGetPhysicalAddress(Adapter->RxDmaDescPool);
	HW_DMA_Init_Desc_Chain(Adapter->RxDmaDescPool,PhyAddress.LowPart, NIC_MAX_BUSY_RECVS);
	Adapter->RxDmaDescIndex = 0;
	for (index = 0; index < NIC_MAX_BUSY_RECVS; index++)
    {
        PRCB Rcb = &((PRCB)Adapter->RcbMemoryBlock)[index];
		ReturnRCB(Adapter, Rcb);
    }

	HWStartStopTxRx(Adapter, TRUE);
	HwEnableInterrupt(Adapter->PhyAdapter);

	DEBUGP(MP_TRACE, "[%p] <--- NICStartTheDatapath\n", Adapter);
}


VOID
NICStopTheDatapath(
    _In_  PMP_ADAPTER  Adapter)
/*++

Routine Description:

    This function prevents future sends and receives on the data path, then
    prepares the adapter to reach an idle state.

    Although the adapter is entering an idle state, there may still be
    outstanding NBLs that haven't been returned by a protocol.  Call NICIsBusy
    to check if NBLs are still outstanding.

    Runs at IRQL == PASSIVE_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter

Return Value:

    None.

--*/
{
	BOOLEAN fResetCancelled;
    DEBUGP(MP_TRACE, "[%p] ---> NICStopTheDatapath.\n", Adapter);

    PAGED_CODE();

    //
    // Remove this adapter from consideration for future receives.
    //
    MPDetachAdapter(Adapter);

	HWStartStopTxRx(Adapter, FALSE);
	HwDisableInterrupt(Adapter->PhyAdapter);

    //
    // Free any queued send operations
    //
    TXFlushSendQueue(Adapter, NDIS_STATUS_FAILURE);

    //
    // Prevent new calls to NICAsyncResetOrPauseDpc
    //
    fResetCancelled = NdisCancelTimerObject(Adapter->AsyncBusyCheckTimer);

    //
    // Wait for any DPCs (like our reset and recv timers) that were in-progress
    // to run to completion.  This is slightly expensive to call, but we don't
    // mind calling it during MiniportHaltEx, since it's not a performance-
    // sensitive path.
    //
    KeFlushQueuedDpcs();

    //
    // Double-check that there are still no queued send operations
    //
    TXFlushSendQueue(Adapter, NDIS_STATUS_FAILURE);


    DEBUGP(MP_TRACE, "[%p] <--- NICStopTheDatapath.\n", Adapter);
}

ULONG
NICGetFrameTypeFromDestination(
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress)
/*++

Routine Description:

    Reads the network frame's destination address to determine the type
    (broadcast, multicast, etc)

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    DestAddress                 The frame's destination address

Return Value:

    NDIS_PACKET_TYPE_BROADCAST
    NDIS_PACKET_TYPE_MULTICAST
    NDIS_PACKET_TYPE_DIRECTED

--*/
{
    if (NIC_ADDR_IS_BROADCAST(DestAddress))
    {
        return NDIS_PACKET_TYPE_BROADCAST;
    }
    else if(NIC_ADDR_IS_MULTICAST(DestAddress))
    {
        return NDIS_PACKET_TYPE_MULTICAST;
    }
    else
    {
        return NDIS_PACKET_TYPE_DIRECTED;
    }
}

