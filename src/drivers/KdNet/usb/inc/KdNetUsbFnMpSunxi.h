/*++

Copyright (c) Microsoft Corporation

Module Name:

    KdNetUsbFnMp.h

Abstract:

    Defines KDNet UsbFn Miniport Interface. This interface abstracts underlying
    USB Function controller hardware. Interface is designed to be consistent
    with UEFI Usb Function Protocol with goal to simplify its implementation.

Author:

    Karel Daniheka (kareld)

--*/

#pragma once

NTSTATUS
UsbFnMpSunxiInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_ PCHAR LoaderOptions,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device,
    _Out_ PULONG ContextLength,
    _Out_ PULONG DmaLength,
    _Out_ PULONG DmaAlignment);

VOID
UsbFnMpSunxiDeinitializeLibrary();

NTSTATUS
UsbFnMpSunxiGetMemoryRequirements(
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device,
    _Out_ PULONG ContextLength,
    _Out_ PULONG DmaLength,
    _Out_ PULONG DmaAlignment);

ULONG
UsbFnMpSunxiGetHardwareContextSize(
    _In_ PDEBUG_DEVICE_DESCRIPTOR Device);

NTSTATUS
UsbFnMpSunxiStartController(
    _In_ PVOID Miniport,
    _Inout_ PDEBUG_DEVICE_DESCRIPTOR Device,
    _In_ PVOID DmaBuffer);

NTSTATUS
UsbFnMpSunxiStopController(
    _In_ PVOID Miniport);
    
NTSTATUS
UsbFnMpSunxiAllocateTransferBuffer(
    _In_ PVOID Miniport,
    ULONG Size,
    _Out_ PVOID* TransferBuffer);

NTSTATUS
UsbFnMpSunxiFreeTransferBuffer(
    _In_ PVOID Miniport,
    _In_ PVOID TransferBuffer);

NTSTATUS
UsbFnMpSunxiConfigureEnableEndpoints(
    _In_ PVOID Miniport,
    _In_ PUSBFNMP_DEVICE_INFO DeviceInfo);

NTSTATUS
UsbFnMpSunxiGetEndpointMaxPacketSize(
    _In_ PVOID Miniport,
    UINT8 EndpointType,
    USB_DEVICE_SPEED BusSpeed,
    _Out_ PUINT16 MaxPacketSize);

NTSTATUS
UsbFnMpSunxiGetMaxTransferSize(
    _In_ PVOID Miniport,
    _Out_ PULONG MaxTransferSize);

NTSTATUS
UsbFnMpSunxiGetEndpointStallState(
    _In_ PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction,
    _Out_ PBOOLEAN Stalled);

NTSTATUS
UsbFnMpSunxiSetEndpointStallState(
    _In_ PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction,
    BOOLEAN StallEndPoint);

NTSTATUS
UsbFnMpSunxiTransfer(
    _In_ PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction,
    _Inout_ PULONG BufferSize,
    _Inout_ PVOID Buffer);

NTSTATUS
UsbFnMpSunxiAbortTransfer(
    _In_ PVOID Miniport,
    UINT8 EndpointIndex,
    USBFNMP_ENDPOINT_DIRECTION Direction);

NTSTATUS
UsbFnMpSunxiEventHandler(
    _In_ PVOID Miniport,
    _Out_ PUSBFNMP_MESSAGE Message,
    _Inout_ PULONG PayloadSize,
    _Out_writes_to_(1, 0) PUSBFNMP_MESSAGE_PAYLOAD Payload);

NTSTATUS
UsbFnMpSunxiTimeDelay(
    _In_ PVOID Miniport,
    _Out_opt_ PULONG Delta,
    _Inout_updates_to_(0, 0) PULONG Base);

