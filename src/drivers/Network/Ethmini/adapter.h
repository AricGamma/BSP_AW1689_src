/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Adapter.H

Abstract:

    This module contains structure definitons and function prototypes.

Revision History:

Notes:

--*/




//
// Utility macros
// -----------------------------------------------------------------------------
//

#define MP_SET_FLAG(_M, _F)             ((_M)->Flags |= (_F))
#define MP_CLEAR_FLAG(_M, _F)           ((_M)->Flags &= ~(_F))
#define MP_TEST_FLAG(_M, _F)            (((_M)->Flags & (_F)) != 0)
#define MP_TEST_FLAGS(_M, _F)           (((_M)->Flags & (_F)) == (_F))

#define MP_IS_READY(_M)        (((_M)->Flags &                             \
                                 (fMP_DISCONNECTED                         \
                                    | fMP_RESET_IN_PROGRESS                \
                                    | fMP_ADAPTER_HALT_IN_PROGRESS         \
                                    | fMP_ADAPTER_PAUSE_IN_PROGRESS        \
                                    | fMP_ADAPTER_PAUSED                   \
                                    | fMP_ADAPTER_LOW_POWER                \
                                    )) == 0)

#define DBGLOG_SIZE (PAGE_SIZE * 16)

typedef struct _MP_DBGLOG_ENTRY
{
	ULONG_PTR Sig;
	ULONG_PTR Data1;
	ULONG_PTR Data2;
	ULONG_PTR Data3;
}DBGLOG_ENTRY, *PDBGLOG_ENTRY;

typedef struct _MP_DBGLOG
{
	ULONG Index;
	ULONG IndexMask;
	PDBGLOG_ENTRY LogStart;
	PDBGLOG_ENTRY LogCurrent;
	PDBGLOG_ENTRY LogEnd;
}DBGLOG, *PDBGLOG;

//
// Each adapter managed by this driver has a MP_ADAPTER struct.
//
typedef struct _MP_ADAPTER
{
    LIST_ENTRY              List;

    //
    // Keep track of various device objects.
    //
    PDEVICE_OBJECT          Pdo;
    PDEVICE_OBJECT          Fdo;
    PDEVICE_OBJECT          NextDeviceObject;

    NDIS_HANDLE             AdapterHandle;

	// Physical Hardware Device defination

	struct _ADAPTER_HW      *PhyAdapter;


    //
    // Status flags
    //

#define fMP_RESET_IN_PROGRESS               0x00000001
#define fMP_DISCONNECTED                    0x00000002
#define fMP_ADAPTER_HALT_IN_PROGRESS        0x00000004
#define fMP_ADAPTER_PAUSE_IN_PROGRESS       0x00000010
#define fMP_ADAPTER_PAUSED                  0x00000020
#define fMP_ADAPTER_SURPRISE_REMOVED        0x00000100
#define fMP_ADAPTER_LOW_POWER               0x00000200

    ULONG                   Flags;


    UCHAR                   PermanentAddress[NIC_MACADDR_SIZE];
    UCHAR                   CurrentAddress[NIC_MACADDR_SIZE];

    //
    // Send tracking
    // -------------------------------------------------------------------------
    //

    // Pool of unused TCBs
    PVOID                   TcbMemoryBlock;

    // List of unused TCBs (sliced out of TcbMemoryBlock)
    LIST_ENTRY              FreeTcbList;
    NDIS_SPIN_LOCK          FreeTcbListLock;

    // List of net buffers to send that are waiting for a free TCB
    LIST_ENTRY              SendWaitList;
    NDIS_SPIN_LOCK          SendWaitListLock;

    // List of TCBs that are being read by the NIC hardware
    LIST_ENTRY              BusyTcbList;
    NDIS_SPIN_LOCK          BusyTcbListLock;

	// DMA Descriptor for TX
	PDMA_DESC				TxDmaDescPool;
	PVOID					TxDataBuffer;
	ULONG					FirstFreeTxDMAIndex;
	ULONG					FirstBusyTxDMAIndex;
	ULONG					TxDmaUsed;
	#define 				INC_TX_DMA_INDEX(x) (((x) + (1)) % NIC_MAX_BUSY_SENDS)

    // Number of transmit NBLs from the protocol that we still have
    volatile LONG           nBusySend;
	volatile LONG           nBusyInd;
	ULONG					nTxFrame;
	ULONG					nRxFrame;

    // Spin lock to ensure only one CPU is sending at a time
    KSPIN_LOCK              SendPathSpinLock;


    //
    // Receive tracking
    // -------------------------------------------------------------------------
    //

    // Pool of unused RCBs
    PVOID                   RcbMemoryBlock;

    // List of unused RCBs (sliced out of RcbMemoryBlock)
    LIST_ENTRY              RcbIndList;
    NDIS_SPIN_LOCK          RcbIndListLock;

    NDIS_HANDLE             RecvNblPoolHandle;

	// DMA Descriptor for RX
	PDMA_DESC				RxDmaDescPool;
	ULONG					RxDmaDescIndex;
	PVOID					RxDataBuffer;
	#define 				INC_RX_DMA_INDEX(x) (((x) + (1)) % NIC_MAX_BUSY_RECVS)

    //
    // Async pause and reset tracking
    // -------------------------------------------------------------------------
    //
    NDIS_HANDLE             AsyncBusyCheckTimer;
    LONG                    AsyncBusyCheckCount;


    //
    // NIC configuration
    // -------------------------------------------------------------------------
    //
    ULONG                   PacketFilter;
    ULONG                   ulLookahead;
    ULONG64                 ulLinkSendSpeed;
    ULONG64                 ulLinkRecvSpeed;
    ULONG                   ulMaxBusySends;
    ULONG                   ulMaxBusyRecvs;

    // multicast list
    ULONG                   ulMCListSize;
    UCHAR                   MCList[NIC_MAX_MCAST_LIST][NIC_MACADDR_SIZE];


    //
    // Statistics
    // -------------------------------------------------------------------------
    //

    // Packet counts
    ULONG64                 FramesRxDirected;
    ULONG64                 FramesRxMulticast;
    ULONG64                 FramesRxBroadcast;
    ULONG64                 FramesTxDirected;
    ULONG64                 FramesTxMulticast;
    ULONG64                 FramesTxBroadcast;

    // Byte counts
    ULONG64                 BytesRxDirected;
    ULONG64                 BytesRxMulticast;
    ULONG64                 BytesRxBroadcast;
    ULONG64                 BytesTxDirected;
    ULONG64                 BytesTxMulticast;
    ULONG64                 BytesTxBroadcast;

    // Count of transmit errors
    ULONG                   TxAbortExcessCollisions;
    ULONG                   TxLateCollisions;
    ULONG                   TxDmaUnderrun;
    ULONG                   TxLostCRS;
    ULONG                   TxOKButDeferred;
    ULONG                   OneRetry;
    ULONG                   MoreThanOneRetry;
    ULONG                   TotalRetries;
    ULONG                   TransmitFailuresOther;

    // Count of receive errors
    ULONG                   RxCrcErrors;
    ULONG                   RxAlignmentErrors;
    ULONG                   RxResourceErrors;
    ULONG                   RxDmaOverrunErrors;
    ULONG                   RxCdtFrames;
    ULONG                   RxRuntErrors;

    //
    // Reference to the allocated root of MP_ADAPTER memory, which may not be cache aligned.
    // When allocating, the pointer returned will be UnalignedBuffer + an offset that will make
    // the base pointer cache aligned.
    //
    PVOID                   UnalignedAdapterBuffer;
    ULONG                   UnalignedAdapterBufferSize;

    //
    // An OID request that could not be fulfulled at the time of the call. These OIDs are serialized
    // so we will not receive new queue management OID's until this one is complete.
    // Currently this is used only for freeing a Queue (which may still have outstanding references)
    //
    PNDIS_OID_REQUEST PendingRequest;

    NDIS_DEVICE_POWER_STATE CurrentPowerState;

	//
	// Debug Log
	//
	struct
	{
		DBGLOG Log;
		UCHAR LogEntry[DBGLOG_SIZE];
	}Debug;

} MP_ADAPTER, *PMP_ADAPTER;

#define MP_ADAPTER_FROM_CONTEXT(_ctx_) ((PMP_ADAPTER)(_ctx_))

NDIS_STATUS
NICAllocRCBData(
    _In_ PMP_ADAPTER Adapter,
    ULONG NumberOfRcbs);

NDIS_STATUS
NICAllocTCBData(
    _In_ PMP_ADAPTER Adapter,
    ULONG NumberOfTcbs
    );

BOOLEAN
NICIsBusy(
    _In_  PMP_ADAPTER  Adapter);

VOID
NICInitializeDbgLog(
    _In_  PMP_ADAPTER  Adapter);

VOID
NICAddDbgLog(
    _Inout_  PMP_ADAPTER  Adapter,
    _In_ ULONG Sig,
    _In_ ULONG Data1,
    _In_ ULONG Data2,
    _In_ ULONG Data3);


// Prototypes for standard NDIS miniport entry points
MINIPORT_INITIALIZE                 MPInitializeEx;
MINIPORT_HALT                       MPHaltEx;
MINIPORT_UNLOAD                     DriverUnload;
MINIPORT_PAUSE                      MPPause;
MINIPORT_RESTART                    MPRestart;
MINIPORT_SEND_NET_BUFFER_LISTS      MPSendNetBufferLists;
MINIPORT_RETURN_NET_BUFFER_LISTS    MPReturnNetBufferLists;
MINIPORT_CANCEL_SEND                MPCancelSend;
MINIPORT_CHECK_FOR_HANG             MPCheckForHangEx;
MINIPORT_RESET                      MPResetEx;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY    MPDevicePnpEventNotify;
MINIPORT_SHUTDOWN                   MPShutdownEx;
MINIPORT_CANCEL_OID_REQUEST         MPCancelOidRequest;

