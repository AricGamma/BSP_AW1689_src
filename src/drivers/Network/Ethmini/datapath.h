/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   DataPath.H

Abstract:

    This module declares the TCB and RCB structures, and the functions to
    manipulate them.

    See the comments in TcbRcb.c.

--*/

#ifndef _DATAPATH_H
#define _DATAPATH_H

NDIS_STATUS
TXNblReference(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER_LIST  NetBufferList);

VOID
TXNblRelease(
    _In_  PMP_ADAPTER       Adapter,
    _In_  PNET_BUFFER_LIST  NetBufferList,
    _In_  BOOLEAN           fAtDispatch);

VOID
TXFlushSendQueue(
    _In_  PMP_ADAPTER  Adapter,
    _In_  NDIS_STATUS  CompleteStatus);

VOID
NICStartTheDatapath(
    _In_  PMP_ADAPTER  Adapter);

VOID
NICStopTheDatapath(
    _In_  PMP_ADAPTER  Adapter);

ULONG
NICGetFrameTypeFromDestination(
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR  DestAddress);

NDIS_STATUS
TxNetBufferCheck(_In_  PNET_BUFFER  NetBuffer);

BOOLEAN
RXReceiveIndicate(
    _In_ PMP_ADAPTER Adapter,
    _In_ ULONG maxNblsToIndicate);

VOID
TXSendComplete(
    _In_ PMP_ADAPTER Adapter);

#endif // _DATAPATH_H

