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

#include <pch.h>
#include <logger.h>

//------------------------------------------------------------------------------

#define DETACH_STALL            20000           // 20 ms

#define STOP_TIMEOUT            1000            // 1 ms
#define RESET_TIMEOUT           1000            // 1 ms
#define PRIME_TIMEOUT           1000            // 1 ms

#define FLUSH_COUNTER           1000000
#define FLUSH_RETRY             8

#define CONFIGURE_TIMEOUT       4000000         // 4 seconds
#define TX_TIMEOUT              1000000         // 1 second
#define RX_TIMEOUT              8000000         // 8 seconds

#define RESTART_COUNTER         4096

//------------------------------------------------------------------------------

#define INREG32(x)              READ_REGISTER_ULONG((PULONG)x)
#define OUTREG32(x, y)          WRITE_REGISTER_ULONG((PULONG)x, y)
#define SETREG32(x, y)          OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)          OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)      OUTREG32(x, (INREG32(x)&~(y))|(z))

//------------------------------------------------------------------------------

#define INVALID_INDEX           (ULONG)-1

#ifndef countof
#define countof(x)              (sizeof(x)/sizeof(x[0]))
#endif

//------------------------------------------------------------------------------

#pragma pack(push, 1)

#define MAX_OEM_WRITE_ENTRIES               8
#define DEBUG_DEVICE_OEM_PORT_USBFN         0x0001
#define OEM_SIGNATURE                       'FIX1'

typedef struct {
    UINT16 PortType;
    UINT16 Reserved;
    UINT32 Signature;
    UINT32 WriteCount;
    struct {
        UINT8 BaseAddressRegister;
        UINT8 Reserved;
        UINT16 Offset;
        UINT32 AndValue;
        UINT32 OrValue;
    } Data[MAX_OEM_WRITE_ENTRIES];
} ACPI_DEBUG_DEVICE_OEM_DATA, *PACPI_DEBUG_DEVICE_OEM_DATA;

#pragma pack(pop)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

UCHAR
gOemData[OEM_BUFFER_SIZE];

ULONG
gOemDataLength;

//------------------------------------------------------------------------------
__inline
static
PHYSICAL_ADDRESS
DmaPa(
    _In_ PUSBFNMP_CONTEXT Context,
    _In_ PVOID VirtualAddress)
{
    PHYSICAL_ADDRESS pa = Context->DmaPa;
    pa.QuadPart += (PUCHAR)VirtualAddress - (PUCHAR)Context->Dma;
    return pa;
}

//------------------------------------------------------------------------------

__inline
static
ULONG
BufferIndexFromAddress(
    PUSBFNMP_CONTEXT Context,
    PVOID Buffer
    )
{
    ULONG Offset, Index;

    Offset = (ULONG)((PUINT8)Buffer - DMA_BUFFER_OFFSET - (PUINT8)Context->Dma->Buffers);
    Index = Offset / DMA_BUFFER_SIZE;
    if ((Index >= DMA_BUFFERS) || ((Offset - Index * DMA_BUFFER_SIZE) != 0)) {
        Index = INVALID_INDEX;
    }
    return Index;
}

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiGetMemoryRequirements(
    PDEBUG_DEVICE_DESCRIPTOR Device,
    PULONG ContextLength,
    PULONG DmaLength,
    PULONG DmaAlignment)
{
    UNREFERENCED_PARAMETER(Device);

    *ContextLength = sizeof(USBFNMP_CONTEXT);
    *DmaLength = sizeof(DMA_MEMORY);
    *DmaAlignment = PAGE_SIZE;

    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiInitializeLibrary(
    PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    PCHAR LoaderOptions,
    PDEBUG_DEVICE_DESCRIPTOR Device,
    PULONG ContextLength,
    PULONG DmaLength,
    PULONG DmaAlignment)
{
    UNREFERENCED_PARAMETER(ImportTable);
    UNREFERENCED_PARAMETER(LoaderOptions);

    return UsbFnMpSunxiGetMemoryRequirements(Device,
                                                ContextLength,
                                                DmaLength,
                                                DmaAlignment);
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
ULONG
UsbFnMpSunxiGetHardwareContextSize(
    PDEBUG_DEVICE_DESCRIPTOR Device)
{
    ULONG ContextLength;
    ULONG DmaLength;
    ULONG DmaAlignment;

    ContextLength = 0;
    DmaLength = 0;
    DmaAlignment = 0;
    UsbFnMpSunxiGetMemoryRequirements(Device,
                                         &ContextLength,
                                         &DmaLength,
                                         &DmaAlignment);

    ContextLength += (PAGE_SIZE - 1);
    ContextLength &= (PAGE_SIZE - 1);
    DmaLength += (PAGE_SIZE - 1);
    DmaLength &= (PAGE_SIZE - 1);

    return (ContextLength + DmaLength);
}

//------------------------------------------------------------------------------

VOID
UsbFnMpSunxiDeinitializeLibrary()
{
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiStartController(
    PVOID Miniport,
    PDEBUG_DEVICE_DESCRIPTOR Device,
    PVOID DmaBuffer)
{
    PUSBFNMP_CONTEXT Context = Miniport;
    ULONG EndpointIndex, BufferIndex;
    NTSTATUS Status;
    INT Ret = 0;

    // Setup miniport context
    if (Device != NULL) {
        Context->UsbCtrl = (PVOID)Device->BaseAddress[0].TranslatedAddress;

        Context->Gpio = NULL;

        Context->Dma = DmaBuffer;
        Context->DmaPa = KdGetPhysicalAddress(DmaBuffer);

        Context->VendorID = Device->VendorID;
        Context->DeviceID = Device->DeviceID;
        Context->NameSpace = Device->NameSpace;

        //
        // Data on OemData pointer is accessible only on first initialization,
        // so make our own copy for subsequent initializations.
        //
        if ((gOemDataLength == 0) && (Device->OemData != NULL) &&
            (Device->OemDataLength < OEM_BUFFER_SIZE)) {
            RtlCopyMemory(gOemData, Device->OemData, Device->OemDataLength);
            gOemDataLength = Device->OemDataLength;
        }

    }

    Context->Attached = FALSE;
    Context->Configured = FALSE;
    Context->PendingReset = FALSE;
    Context->PendingDetach = FALSE;
    Context->Speed = UsbBusSpeedUnknown;

    RtlZeroMemory(Context->Dma, sizeof(*Context->Dma));

    for (BufferIndex = 0; BufferIndex < DMA_BUFFERS; BufferIndex++) {
        Context->NextBuffer[BufferIndex] = BufferIndex + 1;
    }
    Context->NextBuffer[DMA_BUFFERS - 1] = INVALID_INDEX;
    Context->FreeBuffer = 0;

    for (EndpointIndex = 0; EndpointIndex < ENDPOINTS; EndpointIndex++) {
        Context->FirstBuffer[EndpointIndex] = INVALID_INDEX;
        Context->LastBuffer[EndpointIndex] = INVALID_INDEX;
    }

    sunxi_udc_probe();
    Ret = sunxi_start_udc(Miniport, Device, DmaBuffer);
    if (Ret != 0)
        Status = STATUS_NO_SUCH_DEVICE;
    else
        Status = STATUS_SUCCESS;

    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiStopController(
    PVOID Miniport)
{
    NTSTATUS Status;
    INT Ret = 0;

    Ret = sunxi_stop_udc(Miniport);
    if (Ret != 0)
        Status = STATUS_NO_SUCH_DEVICE;
    else
        Status = STATUS_SUCCESS;
    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiAllocateTransferBuffer(
    PVOID Miniport,
    ULONG Size,
    PVOID* TransferBuffer)
{
    NTSTATUS Status;
    PUSBFNMP_CONTEXT Context = Miniport;
    ULONG Index;

    Index = Context->FreeBuffer;
    if ((Size > DMA_BUFFER_SIZE) || (Index == INVALID_INDEX)) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto UsbFnMpAllocateTransferBufferEnd;
    }

    Context->FreeBuffer = Context->NextBuffer[Index];
    Context->NextBuffer[Index] = INVALID_INDEX;
    *TransferBuffer = &Context->Dma->Buffers[Index][DMA_BUFFER_OFFSET];

    Status = STATUS_SUCCESS;

UsbFnMpAllocateTransferBufferEnd:
    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiFreeTransferBuffer(
    PVOID Miniport,
    PVOID TransferBuffer)
{
    NTSTATUS Status;
    PUSBFNMP_CONTEXT Context = Miniport;
    ULONG BufferIndex;

    BufferIndex = BufferIndexFromAddress(Context, TransferBuffer);
    if (BufferIndex == INVALID_INDEX) {
        Status = STATUS_INVALID_PARAMETER;
        goto UsbFnMpFreeTransferBufferEnd;
    }

    Status = STATUS_SUCCESS;

UsbFnMpFreeTransferBufferEnd:
    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiConfigureEnableEndpoints(
    PVOID Miniport,
    PUSBFNMP_DEVICE_INFO DeviceInfo)
{
    UNREFERENCED_PARAMETER(Miniport);

    NTSTATUS Status;
    INT Ret = 0;
    PUSBFNMP_CONFIGURATION_INFO ConfigurationInfo;
    ULONG Interfaces, InterfaceIndex;
    PUSBFNMP_INTERFACE_INFO InterfaceInfo;
    ULONG Endpoints, EndpointIndex;
    PUSB_ENDPOINT_DESCRIPTOR Descriptor;

    // There should be at least one configuration
    if (DeviceInfo->DeviceDescriptor->bNumConfigurations < 1) {
	Status = STATUS_INVALID_PARAMETER;
	goto UsbFnMpConfigureEnableEndpointsEnd;
    }

    // Shortcut

    // Iterate over all interfaces in first configuration.
    ConfigurationInfo = DeviceInfo->ConfigurationInfoTable[0];
    Interfaces = ConfigurationInfo->ConfigDescriptor->bNumInterfaces;
    for (InterfaceIndex = 0; InterfaceIndex < Interfaces; InterfaceIndex++) {
	InterfaceInfo = ConfigurationInfo->InterfaceInfoTable[InterfaceIndex];

	// Iterate over all endpoints in this interface.
	Endpoints = InterfaceInfo->InterfaceDescriptor->bNumEndpoints;
	for (EndpointIndex = 0; EndpointIndex < Endpoints; EndpointIndex++) {
	    Descriptor = InterfaceInfo->EndpointDescriptorTable[EndpointIndex];

	    // Configure and enable end point
	    Ret = sunxi_udc_ep_enable(Descriptor);
            if (Ret != 0) {
                Status = STATUS_INVALID_PARAMETER;
                goto UsbFnMpConfigureEnableEndpointsEnd;
            }
	}
    }

    // We are now configured

    // Done
    Status = STATUS_SUCCESS;

UsbFnMpConfigureEnableEndpointsEnd:

    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiGetEndpointMaxPacketSize(
    PVOID Miniport,
    UINT8 EndpointType,
    USB_DEVICE_SPEED BusSpeed,
    PUINT16 MaxPacketSize)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Miniport);

    switch (BusSpeed) {
        case UsbLowSpeed:
        case UsbSuperSpeed:
        default:
            *MaxPacketSize = 0;
            Status = STATUS_INVALID_PARAMETER;
            break;
        case UsbFullSpeed:
            switch (EndpointType) {
            case USB_ENDPOINT_TYPE_CONTROL:
            case USB_ENDPOINT_TYPE_BULK:
            case USB_ENDPOINT_TYPE_INTERRUPT:
                *MaxPacketSize = 64;
                Status = STATUS_SUCCESS;
                break;
            case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                *MaxPacketSize = 1023;
                Status = STATUS_SUCCESS;
                break;
            default:
                *MaxPacketSize = 0;
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            break;
        case UsbHighSpeed:
            switch (EndpointType) {
            case USB_ENDPOINT_TYPE_CONTROL:
                *MaxPacketSize = 64;
                Status = STATUS_SUCCESS;
                break;
            case USB_ENDPOINT_TYPE_BULK:
                *MaxPacketSize = 512;
                Status = STATUS_SUCCESS;
                break;
            case USB_ENDPOINT_TYPE_INTERRUPT:
                *MaxPacketSize = 1024;
                Status = STATUS_SUCCESS;
                break;
            case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                *MaxPacketSize = 1024;
                Status = STATUS_SUCCESS;
                break;
            default:
                *MaxPacketSize = 0;
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            break;
    }

    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiGetMaxTransferSize(
    PVOID Miniport,
    PULONG MaxTransferSize)
{
    UNREFERENCED_PARAMETER(Miniport);

    *MaxTransferSize = DMA_BUFFER_SIZE;
    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiGetEndpointStallState(
    PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction,
    PBOOLEAN Stalled)
{
//Miniport
    UNREFERENCED_PARAMETER(Miniport);
    sunxi_udc_get_halt(EndpointIndex, Direction, Stalled);
    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiSetEndpointStallState(
    PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction,
    BOOLEAN StallEndPoint)
{
//Miniport
    UNREFERENCED_PARAMETER(Miniport);
    NTSTATUS Status;
    INT Ret = 0;

    Ret = sunxi_udc_set_halt(EndpointIndex, Direction, StallEndPoint);
 
    if (Ret != 0)
        Status = STATUS_NO_SUCH_DEVICE;
    else
        Status = STATUS_SUCCESS;

    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiTransfer(
    PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction,
    PULONG BufferSize,
    PVOID Buffer)
{
    NTSTATUS Status;
    INT Ret = 0;
    PUSBFNMP_CONTEXT Context = Miniport;
    ULONG BufferIndex, EndpointPos;
    PHYSICAL_ADDRESS pa;

    // Get buffer index
    BufferIndex = BufferIndexFromAddress(Context, Buffer);
    if (BufferIndex == INVALID_INDEX) {
        Status = STATUS_INVALID_PARAMETER;
        goto UsbFnMpTransferEnd;
    }

    // Get end point index
    EndpointPos = EndpointIndex << 1;
    if (Direction == UsbEndpointDirectionDeviceTx) EndpointPos++;
    if (EndpointPos >= ENDPOINTS) {
        Status = STATUS_INVALID_PARAMETER;
        goto UsbFnMpTransferEnd;
    }

    pa = DmaPa(Context, Buffer);

    Ret = sunxi_udc_queue(EndpointIndex, Direction, BufferSize, Buffer, pa,
                          BufferIndex);
    if (Ret != 0)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = STATUS_SUCCESS;

UsbFnMpTransferEnd:

    return Status;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiAbortTransfer(
    PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction)
{
    UNREFERENCED_PARAMETER(Miniport);
    UNREFERENCED_PARAMETER(EndpointIndex);
    UNREFERENCED_PARAMETER(Direction);

    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiEventHandler(
    PVOID Miniport,
    PUSBFNMP_MESSAGE Message,
    PULONG PayloadSize,
    PUSBFNMP_MESSAGE_PAYLOAD Payload)
{
    UNREFERENCED_PARAMETER(Miniport);
    // No event to report
    *Message = UsbMsgNone;
    *PayloadSize = 0;

    sunxi_udc_irq(Message, Payload);

    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------

_Use_decl_annotations_
NTSTATUS
UsbFnMpSunxiTimeDelay(
    PVOID Miniport,
    PULONG Delta,
    PULONG Base)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(Miniport);
    UNREFERENCED_PARAMETER(Delta);
    UNREFERENCED_PARAMETER(Base);

    if (Delta != NULL)
        *Delta = 0;

    return Status;
}

//------------------------------------------------------------------------------

