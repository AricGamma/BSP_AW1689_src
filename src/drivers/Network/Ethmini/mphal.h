/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   MpHAL.H

Abstract:

    This module declares the structures and functions that abstract the
    adapter and medium's (emulated) hardware capabilities.

--*/


#ifndef _MPHAL_H
#define _MPHAL_H

// HW device object
typedef struct _ADAPTER_HW
{
	ULONG Size;
	PMP_ADAPTER Adapter;
	
	//Resources
	CM_PARTIAL_RESOURCE_DESCRIPTOR rPort;
	CM_PARTIAL_RESOURCE_DESCRIPTOR rInterrupt;
    CM_PARTIAL_RESOURCE_DESCRIPTOR rMemory;
	
    // register space.
    struct {
        PHYSICAL_ADDRESS PhysicalBase;
        PVOID VirtualBase;
        ULONG Length;
    } Register;

	struct {
		PHYSICAL_ADDRESS PhysicalBase;
		PVOID VirtualBase;
		ULONG Length;
	} MdioPinRegister;

	NDIS_HANDLE                 InterruptHandle;
	ULONG						IntStatus;

	MAC			Mac;

}HWADAPTER, *PHWADAPTER;

struct _TCB;
struct _RCB;

NDIS_STATUS FreePhyAdapter
(_In_  PHWADAPTER                   pPhyAdapter);

NDIS_STATUS
HWInitialize(
    _In_  PMP_ADAPTER Adapter,
    _In_  NDIS_HANDLE  WrapperConfigurationContext);

VOID
HWReadPermanentMacAddress(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_HANDLE  ConfigurationHandle,
    _Out_writes_bytes_(NIC_MACADDR_SIZE)  PUCHAR  PermanentMacAddress);

NDIS_STATUS
HWGetDestinationAddress(
    _In_  PNET_BUFFER  NetBuffer,
    _Out_writes_bytes_(NIC_MACADDR_SIZE) PUCHAR DestAddress);

BOOLEAN
HWIsFrameAcceptedByPacketFilter(
    _In_  PMP_ADAPTER  Adapter,
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress,
    _In_  ULONG        FrameType);

NDIS_MEDIA_CONNECT_STATE
HWGetMediaConnectStatus(
    _In_  PMP_ADAPTER Adapter);

VOID
HWProgramDmaForSend(
    _In_  PMP_ADAPTER   Adapter,
    _In_  struct _TCB  *Tcb);

NDIS_STATUS
HWReceiveDma(
    _In_  PMP_ADAPTER   Adapter,
    _In_  struct _RCB *Rcb);

NDIS_STATUS
HWStartStopTxRx(
    _In_  PMP_ADAPTER  Adapter,
    _In_  BOOLEAN      Start);

VOID Dbg_Dump_Data(BOOLEAN Tx, ULONG Frame, PUCHAR Data, ULONG Length);

#endif // _MPHAL_H

