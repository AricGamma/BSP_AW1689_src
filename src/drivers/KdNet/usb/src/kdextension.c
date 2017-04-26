/*++

Copyright (c) Microsoft Corporation

Module Name:

    kdextension.c

Abstract:

    Network Kernel Debug Extensibility Support.  This module implements the
    extensibility entry points.

Author:

    Joe Ballantyne (joeball)
    Fredrik Grohn (fgrohn)
    Karel Danihelka (kareld)

--*/

#include "pch.h"
#include <logger.h>
#include "KdNetUsbFnMpSunxi.h"
#include "kdnetusbfnb.h"

//------------------------------------------------------------------------------

#define KDQCOM_SUBTYPE_CHIPIDEA_USBFN       1
#define KDQCOM_SUBTYPE_CHIPIDEA_AX88772     2
#define KDQCOM_SUBTYPE_CHIPIDEA_USBFNB      3
#define KDQCOM_SUBTYPE_SYNOPSYS_USBFN       4
#define KDQCOM_SUBTYPE_SYNOPSYS_USBFNB      5
#define KDALLWINNER_SUBTYPE_SUNXI_USBFNB    6

PKDNET_EXTENSIBILITY_IMPORTS KdNetExtensibilityImports;
KDNET_EXTENSIBILITY_EXPORTS KdNetExtensibilityExports;
KDNET_USBFNMP_EXPORTS KdNetUsbFnMpExports;

//------------------------------------------------------------------------------

VOID
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is a dummy function that allows the extensibility module to
    link against the kernel mode GS buffer overflow library.

Arguments:

    DriverObject - Supplies a driver object.

    RegistryPath - Supplies the driver's registry path.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
}

//------------------------------------------------------------------------------

VOID
KdSetHibernateRange (
    VOID
    )
{
    //
    // Mark kdnet extension code.
    //

    PoSetHiberRange(NULL,
                    PO_MEM_BOOT_PHASE,
                    (PVOID)(ULONG_PTR)KdSetHibernateRange,
                    0,
                    'endK');
}

//------------------------------------------------------------------------------

NTSTATUS
KdInitializeController(
    __inout PKDNET_SHARED_DATA KdNet
    )
{
    NTSTATUS Status;
    USHORT PortType;

    if (KdNetExtensibilityExports.KdInitializeController != NULL)
        {
        Status = KdNetExtensibilityExports.KdInitializeController(KdNet);
        goto KdInitializeControllerEnd;
        }

    if ((KdNet != NULL) &&
        (KdNet->Device != NULL) &&
        (KdNet->Device->OemData != NULL) &&
        (KdNet->Device->OemDataLength >= sizeof(USHORT))) {

        PortType = *(PUSHORT)KdNet->Device->OemData;
        }
    else
        {
        PortType = KDALLWINNER_SUBTYPE_SUNXI_USBFNB;
        }

    switch (PortType)
        {
        case KDALLWINNER_SUBTYPE_SUNXI_USBFNB:
            KdNetExtensibilityExports.KdInitializeController = KdUsbFnBInitializeController;
            KdNetExtensibilityExports.KdShutdownController = KdUsbFnBShutdownController;
            KdNetExtensibilityExports.KdGetTxPacket = KdUsbFnBGetTxPacket;
            KdNetExtensibilityExports.KdSendTxPacket = KdUsbFnBSendTxPacket;
            KdNetExtensibilityExports.KdGetRxPacket = KdUsbFnBGetRxPacket;
            KdNetExtensibilityExports.KdReleaseRxPacket = KdUsbFnBReleaseRxPacket;
            KdNetExtensibilityExports.KdGetPacketAddress = KdUsbFnBGetPacketAddress;
            KdNetExtensibilityExports.KdGetPacketLength = KdUsbFnBGetPacketLength;
            KdNetExtensibilityExports.KdGetHardwareContextSize = KdUsbFnBGetHardwareContextSize;
            KdNetUsbFnMpExports.InitializeLibrary = UsbFnMpSunxiInitializeLibrary;
            KdNetUsbFnMpExports.DeinitializeLibrary = UsbFnMpSunxiDeinitializeLibrary;
            KdNetUsbFnMpExports.GetHardwareContextSize = UsbFnMpSunxiGetHardwareContextSize;
            KdNetUsbFnMpExports.StartController = UsbFnMpSunxiStartController;
            KdNetUsbFnMpExports.StopController = UsbFnMpSunxiStopController;
            KdNetUsbFnMpExports.AllocateTransferBuffer = UsbFnMpSunxiAllocateTransferBuffer;
            KdNetUsbFnMpExports.FreeTransferBuffer = UsbFnMpSunxiFreeTransferBuffer;
            KdNetUsbFnMpExports.ConfigureEnableEndpoints = UsbFnMpSunxiConfigureEnableEndpoints;
            KdNetUsbFnMpExports.GetEndpointMaxPacketSize = UsbFnMpSunxiGetEndpointMaxPacketSize;
            KdNetUsbFnMpExports.GetMaxTransferSize = UsbFnMpSunxiGetMaxTransferSize;
            KdNetUsbFnMpExports.GetEndpointStallState = UsbFnMpSunxiGetEndpointStallState;
            KdNetUsbFnMpExports.SetEndpointStallState = UsbFnMpSunxiSetEndpointStallState;
            KdNetUsbFnMpExports.Transfer = UsbFnMpSunxiTransfer;
            KdNetUsbFnMpExports.AbortTransfer = UsbFnMpSunxiAbortTransfer;
            KdNetUsbFnMpExports.EventHandler = UsbFnMpSunxiEventHandler;
            KdNetUsbFnMpExports.TimeDelay = UsbFnMpSunxiTimeDelay;
            break;
        }

    Status = KdNetExtensibilityExports.KdInitializeController(KdNet);

KdInitializeControllerEnd:
    return Status;
}

//------------------------------------------------------------------------------

VOID
KdShutdownController(
    __inout PVOID Buffer
    )
{
    KdNetExtensibilityExports.KdShutdownController(Buffer);
}

//------------------------------------------------------------------------------

NTSTATUS
KdGetTxPacket(
    __inout PVOID Buffer,
    __out PULONG Handle
    )
{
    return KdNetExtensibilityExports.KdGetTxPacket(Buffer, Handle);
}

//------------------------------------------------------------------------------

NTSTATUS
KdSendTxPacket(
    __inout PVOID Buffer,
    ULONG Handle,
    ULONG Length
    )
{
    return KdNetExtensibilityExports.KdSendTxPacket(Buffer, Handle, Length);
}

//------------------------------------------------------------------------------

NTSTATUS
KdGetRxPacket(
    __inout PVOID Buffer,
    __out PULONG Handle,
    __out PVOID *Packet,
    __out PULONG Length
    )
{
    return KdNetExtensibilityExports.KdGetRxPacket(
        Buffer, Handle, Packet, Length
        );
}

//------------------------------------------------------------------------------

VOID
KdReleaseRxPacket(
    __inout PVOID Buffer,
    ULONG Handle
    )
{
    KdNetExtensibilityExports.KdReleaseRxPacket(Buffer, Handle);
}

//------------------------------------------------------------------------------

PVOID
KdGetPacketAddress (
    __inout PVOID Buffer,
    ULONG Handle
    )
{
    return KdNetExtensibilityExports.KdGetPacketAddress(Buffer, Handle);
}

//------------------------------------------------------------------------------

ULONG
KdGetPacketLength(
    __inout PVOID Buffer,
    ULONG Handle
    )
{
    return KdNetExtensibilityExports.KdGetPacketLength(Buffer, Handle);
}

//------------------------------------------------------------------------------

ULONG
KdGetHardwareContextSize (
    __in PDEBUG_DEVICE_DESCRIPTOR Device
    )

/*++

Routine Description:

    This function returns the required size of the hardware context in bytes.

Arguments:

    Device - Supplies a pointer to the debug device descriptor.

Return Value:

    None.

--*/

{
    return KdNetExtensibilityExports.KdGetHardwareContextSize(Device);
}

//------------------------------------------------------------------------------

NTSTATUS
KdInitializeLibrary (
    __in PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    __in_opt PSTR LoaderOptions,
    __inout PDEBUG_DEVICE_DESCRIPTOR Device
    )
{
    BOOLEAN Aligned;
    BOOLEAN Cached;
    ULONG Length;
    PKDNET_EXTENSIBILITY_EXPORTS Exports;
    NTSTATUS Status = STATUS_SUCCESS;

    __security_init_cookie();

    DebugOutputInit(Device, 1);

    //
    // Check parameters
    //

    if ((ImportTable == NULL) ||
        (ImportTable->FunctionCount != KDNET_EXT_IMPORTS)) {
        Status = STATUS_INVALID_PARAMETER;
        goto KdInitializeLibraryEnd;
    }

    // Save import jump table
    KdNetExtensibilityImports = ImportTable;

    Exports = KdNetExtensibilityImports->Exports;
    if ((Exports == NULL) || (Exports->FunctionCount != KDNET_EXT_EXPORTS)) {
        Status = STATUS_INVALID_PARAMETER;
        goto KdInitializeLibraryEnd;
    }

    //
    // Return the function pointers this KDNET extensibility module exports.
    //

    Exports->KdInitializeController = KdInitializeController;
    Exports->KdShutdownController = KdShutdownController;
    Exports->KdSetHibernateRange = KdSetHibernateRange;
    Exports->KdGetRxPacket = KdGetRxPacket;
    Exports->KdReleaseRxPacket = KdReleaseRxPacket;
    Exports->KdGetTxPacket = KdGetTxPacket;
    Exports->KdSendTxPacket = KdSendTxPacket;
    Exports->KdGetPacketAddress = KdGetPacketAddress;
    Exports->KdGetPacketLength = KdGetPacketLength;
    Exports->KdGetHardwareContextSize = KdGetHardwareContextSize;

    //
    // Start with most optimistic memory requirements.
    //

    Length = Device->Memory.Length = 0;
    Cached = Device->Memory.Cached = TRUE;
    Aligned = Device->Memory.Aligned = FALSE;

    //
    // Call sunxi controller.
    //

    KdNetUsbFnMpExports.InitializeLibrary = UsbFnMpSunxiInitializeLibrary;
    KdNetUsbFnMpExports.GetHardwareContextSize = UsbFnMpSunxiGetHardwareContextSize;
    Status = KdUsbFnInitializeLibrary(ImportTable, LoaderOptions, Device);
    if (NT_SUCCESS(Status)) {
        if (Device->Memory.Length > Length) {
            Length = Device->Memory.Length;
        }

        Cached &= Device->Memory.Cached;
        Aligned |= Device->Memory.Aligned;
    }

    //
    // Update memory requirement.
    //

    Device->Memory.Length = Length;
    Device->Memory.Cached = Cached;
    Device->Memory.Aligned = Aligned;

    Status = STATUS_SUCCESS;

KdInitializeLibraryEnd:
    return Status;
}

//------------------------------------------------------------------------------
