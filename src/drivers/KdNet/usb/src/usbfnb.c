/** @file
*
*  Copyright (c) 2007-2016, Allwinner Technology Co., Ltd. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**/

#include <ntddk.h>
#include <process.h>
#include <kdnetshareddata.h>
#include <kdnetextensibility.h>
#include <kdnetusbfnb.h>
#include <kdnetusbfn.h>
#include <logger.h>

//------------------------------------------------------------------------------

#define INVALID_HANDLE      (ULONG)-1

#define TX_HANDLE           0x00000001
#define RX_HANDLE           0x00000101

#define BUFFER_OFFSET       2
#define BUFFER_SIZE         2048

ULONG
TxHandle;

ULONG
RxHandle;

UCHAR
TxBuffer[BUFFER_SIZE];

#define TX_COOKIE    'TxBg'

ULONG
TxCookie = TX_COOKIE;

UCHAR
RxBuffer[BUFFER_SIZE];

#define RX_COOKIE    'RxBg'

ULONG
RxCookie = RX_COOKIE;

//------------------------------------------------------------------------------

#define ASSERTBG(x, y)     if (!(x))                                           \
    KeBugCheckEx(THREAD_STUCK_IN_DEVICE_DRIVER, 2, y, 0, 0)

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
KdUsbFnBInitializeLibrary (
    PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    PCHAR LoaderOptions,
    PDEBUG_DEVICE_DESCRIPTOR Device
    )
{
    return KdUsbFnInitializeLibrary(ImportTable, LoaderOptions, Device);
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
ULONG
KdUsbFnBGetHardwareContextSize (
    PDEBUG_DEVICE_DESCRIPTOR Device
    )
{
    return KdUsbFnGetHardwareContextSize(Device);
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
KdUsbFnBInitializeController (
    PKDNET_SHARED_DATA KdNet
    )
{
    TxHandle = INVALID_HANDLE;
    RxHandle = INVALID_HANDLE;
    return KdUsbFnInitializeController(KdNet);
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
VOID
KdUsbFnBShutdownController (
    PVOID Buffer
    )
{
    KdUsbFnShutdownController(Buffer);
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
KdUsbFnBGetTxPacket (
    PVOID Buffer,
    PULONG Handle
    )
{
    NTSTATUS Status;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 1);
    ASSERTBG(TxHandle == INVALID_HANDLE, 2);

    // Get packet from normal extension
    Status = KdUsbFnGetTxPacket(Buffer, &TxHandle);
    if (!NT_SUCCESS(Status)) goto KdUsbFnBGetTxPacketEnd;

    *Handle = TX_HANDLE;

KdUsbFnBGetTxPacketEnd:
    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 3);
    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
KdUsbFnBSendTxPacket (
    PVOID Buffer,
    ULONG Handle,
    ULONG Length
    )
{
    NTSTATUS Status;
    PVOID Packet;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 4);

    // Strip extra flags
    Handle &= ~(TRANSMIT_ASYNC | TRANSMIT_HANDLE);
    ASSERTBG((Handle == TX_HANDLE) && (TxHandle != INVALID_HANDLE), 5);

    ASSERTBG(Length <= (sizeof(TxBuffer) - BUFFER_OFFSET), 6);
    if (Length > (sizeof(TxBuffer) - BUFFER_OFFSET)) {
        Length = sizeof(TxBuffer) - BUFFER_OFFSET;
    }        

    Packet = KdUsbFnGetPacketAddress(Buffer, TxHandle);
    RtlCopyMemory(Packet, &TxBuffer[BUFFER_OFFSET], Length);
    Status = KdUsbFnSendTxPacket(Buffer, TxHandle, Length);

    TxHandle = INVALID_HANDLE;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 7);
    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
KdUsbFnBGetRxPacket (
    PVOID Buffer,
    PULONG Handle,
    PVOID *Packet,
    PULONG Length
    )
{
    NTSTATUS Status;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 8);
    ASSERTBG(RxHandle == INVALID_HANDLE, 9);

    Status = KdUsbFnGetRxPacket(Buffer, &RxHandle, Packet, Length);
    if (NT_SUCCESS(Status))
        {
        RtlCopyMemory(&RxBuffer[BUFFER_OFFSET], *Packet, *Length);
        *Handle = RX_HANDLE;
        *Packet = &RxBuffer[BUFFER_OFFSET];
        }

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 10);
    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
VOID
KdUsbFnBReleaseRxPacket (
    PVOID Buffer,
    ULONG Handle
    )
{
    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 11);
    ASSERTBG((Handle == RX_HANDLE) && (RxHandle != INVALID_HANDLE), 12);

    KdUsbFnReleaseRxPacket(Buffer, RxHandle);
    RxHandle = INVALID_HANDLE;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 13);
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
PVOID
KdUsbFnBGetPacketAddress (
    PVOID Buffer,
    ULONG Handle
    )
{
    PVOID Address;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 14);

    UNREFERENCED_PARAMETER(Buffer);

    // Strip extra flags
    Handle &= ~(TRANSMIT_ASYNC | TRANSMIT_HANDLE);

    // Buffer address varies by handle
    if (Handle == TX_HANDLE) {
        Address = &TxBuffer[BUFFER_OFFSET];
    }
    else if (Handle == RX_HANDLE) {
        Address = &RxBuffer[BUFFER_OFFSET];
    }
    else {
        Address = NULL;
    }

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 15);
    return Address;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
ULONG
KdUsbFnBGetPacketLength(
    PVOID Buffer,
    ULONG Handle
    )
{
    ULONG Length;

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 16);

    // Strip extra flags
    Handle &= ~(TRANSMIT_ASYNC | TRANSMIT_HANDLE);

    // Forward request with real handle
    if (Handle == TX_HANDLE) {
        Length = KdUsbFnGetPacketLength(Buffer, TxHandle);
    }
    else if (Handle == RX_HANDLE) {
        Length = KdUsbFnGetPacketLength(Buffer, RxHandle);
    }
    else {
        Length  = 0;
    }

    ASSERTBG((TxCookie != TX_COOKIE) || (RxCookie != TX_COOKIE), 17);
    return Length;
}

//------------------------------------------------------------------------------
