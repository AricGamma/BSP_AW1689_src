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



//
// ------------------------------------------------------------------- Includes
//

#include <ntddk.h>
#include <wdf.h>
#include <gpioclx.h>
#include <acpiioct.h>
#include "Trace.h"

ULONG g_DebugLevel = DEBUG_LEVEL_TERSE;


//
// -------------------------------------------------------------------- Defines
//

//
// Define total number of pins on the sunxi GPIO controller.
//

#define SUNXI_GPIO_PINS_PER_BANK		(32)
#define SUNXI_GPIO_TOTAL_BANKS			(10)


//
// Define that controls whether f-state based power management will be supported
// or not by the client driver.
//

//#define ENABLE_F_STATE_POWER_MGMT
#define MP_TAG_GENERAL              'gPiO'


#define SUNXI_GPIO_F1_NOMINAL_POWER	(0)
#define SUNXI_GPIO_F1_RESIDENCY		(8*60*60)          // 8hrs
#define SUNXI_GPIO_F1_TRANSITION		(1*60*60)         // 1hr


//
// Macro for pointer arithmetic.
//

#define Add2Ptr(Ptr, Value) ((PVOID)((PUCHAR)(Ptr) + (Value)))

//
// Determine whether the given pin is reserved or not. Currently no pins are
// reserved on the simulated GPIO controller.
//

__pragma(warning(disable: 4127))        // conditional expression is a constant

//
// ---------------------------------------------------------------------- Types
//

//
// Define the registers within the sunxigpio controller. There are 32 pins per
// controller. Note this is a logical device and thus may correspond to a
// physical bank or module if the GPIO controller in hardware has more than
// 32 pins.
//



typedef struct _SUNXI_GPIO_REGISTERS {
    ULONG PnCfg[4];
    ULONG PnDat;
    ULONG PnDrv[2];
    ULONG PnPul[2];
} SUNXI_GPIO_REGISTERS, *PSUNXI_GPIO_REGISTERS;

typedef struct _SUNXI_GPIO_INT_REGISTERS {
	ULONG PnIntCfg[4];
	ULONG PnIntCtl;
	ULONG PnIntSta;
	ULONG PnIntDeb;
}SUNXI_GPIO_INT_REGISTERS, *PSUNXI_GPIO_INT_REGISTERS;

typedef struct _SUNXI_GPIO_BANK {
    LARGE_INTEGER			PhysicalBaseAddress;
    PSUNXI_GPIO_REGISTERS		Registers;
	PSUNXI_GPIO_INT_REGISTERS IntRegisters;
    ULONG					AddressLength;
	ULONG					IntLength;
	UCHAR					IsIntBank;
	PIN_NUMBER				NumberOfPins;
    SUNXI_GPIO_REGISTERS		SavedContext;
	SUNXI_GPIO_INT_REGISTERS	IntSavedContext;
	ULONG						InterruptResourceIndex;
} SUNXI_GPIO_BANK, *PSUNXI_GPIO_BANK;

//
// The sunxigpio client driver device extension.
//

typedef struct _SUNXI_GPIO_CONTEXT {
    USHORT				TotalPins;
    LARGE_INTEGER		PhysicalBaseAddress;
    PSUNXI_GPIO_REGISTERS ControllerBase;
    ULONG				AddressLength;
	ULONG				TotalBanks;
	UCHAR				InterruptIndex;
    SUNXI_GPIO_BANK		Banks[SUNXI_GPIO_TOTAL_BANKS];
} SUNXI_GPIO_CONTEXT, *PSUNXI_GPIO_CONTEXT;

typedef struct _ACPI_DATA {
		ACPI_EVAL_OUTPUT_BUFFER buffer;
		ULONG data[7];
} ACPI_DATA, *PACPI_DATA;

SUNXI_GPIO_REGISTERS GlobalGpioRegisters[SUNXI_GPIO_TOTAL_BANKS] = {0};

//
// ----------------------------------------------------------------- Prototypes
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD SunxiGpioEvtDriverUnload;
EVT_WDF_DRIVER_DEVICE_ADD SunxiGpioEvtDeviceAdd;

//
// General interfaces.
//

GPIO_CLIENT_PREPARE_CONTROLLER SunxiGpioPrepareController;
GPIO_CLIENT_RELEASE_CONTROLLER sunxigpioReleaseController;
GPIO_CLIENT_QUERY_CONTROLLER_BASIC_INFORMATION
    SunxiGpioQueryControllerBasicInformation;

GPIO_CLIENT_QUERY_SET_CONTROLLER_INFORMATION
    SunxiGpioQuerySetControllerInformation;

GPIO_CLIENT_START_CONTROLLER SunxiGpioStartController;
GPIO_CLIENT_STOP_CONTROLLER SunxiGpioStopController;

//
// Interrupt enable, disable, mask and unmask handlers.
//

GPIO_CLIENT_ENABLE_INTERRUPT SunxiGpioEnableInterrupt;
GPIO_CLIENT_DISABLE_INTERRUPT SunxiGpioDisableInterrupt;
GPIO_CLIENT_MASK_INTERRUPTS SunxiGpioMaskInterrupts;
GPIO_CLIENT_UNMASK_INTERRUPT SunxiGpioUnmaskInterrupt;
GPIO_CLIENT_RECONFIGURE_INTERRUPT SunxiGpioReconfigureInterrupt;

//
// Handlers to query active/enabled interrupts and clear active interrupts.
//

GPIO_CLIENT_QUERY_ACTIVE_INTERRUPTS SunxiGpioQueryActiveInterrupts;
GPIO_CLIENT_CLEAR_ACTIVE_INTERRUPTS SunxiGpioClearActiveInterrupts;
GPIO_CLIENT_QUERY_ENABLED_INTERRUPTS SunxiGpioQueryEnabledInterrupts;

//
// Handlers for GPIO I/O operations.
//

GPIO_CLIENT_CONNECT_IO_PINS SunxiGpioConnectIoPins;
GPIO_CLIENT_DISCONNECT_IO_PINS SunxiGpioDisconnectIoPins;
GPIO_CLIENT_READ_PINS_MASK SunxiGpioReadGpioPins;
GPIO_CLIENT_WRITE_PINS_MASK SunxiGpioWriteGpioPins;

//
// Handlers for save and restore hardware context callbacks.
//

GPIO_CLIENT_SAVE_BANK_HARDWARE_CONTEXT SunxiGpioSaveBankHardwareContext;
GPIO_CLIENT_RESTORE_BANK_HARDWARE_CONTEXT SunxiGpioRestoreBankHardwareContext;

//
// -------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SunxiGpioEvtDriverUnload)
#pragma alloc_text(PAGE, SunxiGpioEvtDeviceAdd)

//
// ------------------------------------------------------------------ Functions
//

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver initialization entry point.

Arguments:

    DriverObject - Pointer to the driver object created by the I/O manager.

    RegistryPath - Pointer to the driver specific registry key.

Return Value:

    NTSTATUS code.

--*/

{
	NTSTATUS Status;
    WDFDRIVER Driver;
    WDF_DRIVER_CONFIG DriverConfig;
    GPIO_CLIENT_REGISTRATION_PACKET RegistrationPacket;
    
	FunctionEnter();
    //
    // Initialize the driver configuration structure.
    //

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SunxiGpioEvtDeviceAdd);
    DriverConfig.EvtDriverUnload = SunxiGpioEvtDriverUnload;

    //
    // Create a framework driver object to represent our driver.
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             &Driver);

    if (!NT_SUCCESS(Status)) {
        goto DriverEntryEnd;
    }

    //
    // Initialize the client driver registration packet.
    //

    RtlZeroMemory(&RegistrationPacket, sizeof(GPIO_CLIENT_REGISTRATION_PACKET));
    RegistrationPacket.Version = GPIO_CLIENT_VERSION;
    RegistrationPacket.Size = sizeof(GPIO_CLIENT_REGISTRATION_PACKET);

    //
    // Initialize the device context size.
    //

    RegistrationPacket.ControllerContextSize = sizeof(SUNXI_GPIO_CONTEXT);

    //
    // General interfaces.
    //

    RegistrationPacket.CLIENT_PrepareController = SunxiGpioPrepareController;
    RegistrationPacket.CLIENT_QueryControllerBasicInformation = SunxiGpioQueryControllerBasicInformation;

    //
    // The query/set handler is required in this sample only if F-state
    // power management is enabled to indicate which banks support f-state
    // based power maanagement.
    //

    RegistrationPacket.CLIENT_QuerySetControllerInformation =SunxiGpioQuerySetControllerInformation;

    RegistrationPacket.CLIENT_StartController = SunxiGpioStartController;
    RegistrationPacket.CLIENT_StopController = SunxiGpioStopController;
    RegistrationPacket.CLIENT_ReleaseController = sunxigpioReleaseController;

    //
    // Interrupt enable and disable handlers.
    //

    RegistrationPacket.CLIENT_DisableInterrupt = SunxiGpioDisableInterrupt;
    RegistrationPacket.CLIENT_EnableInterrupt = SunxiGpioEnableInterrupt;

    //
    // Interrupt mask, unmask and reconfigure interrupt handlers.
    //

    RegistrationPacket.CLIENT_MaskInterrupts = SunxiGpioMaskInterrupts;
    RegistrationPacket.CLIENT_UnmaskInterrupt = SunxiGpioUnmaskInterrupt;
    RegistrationPacket.CLIENT_ReconfigureInterrupt = SunxiGpioReconfigureInterrupt;

    //
    // Handlers to query active/enabled interrupts and clear active interrupts.
    //

    RegistrationPacket.CLIENT_ClearActiveInterrupts = SunxiGpioClearActiveInterrupts;
    RegistrationPacket.CLIENT_QueryActiveInterrupts = SunxiGpioQueryActiveInterrupts;
    RegistrationPacket.CLIENT_QueryEnabledInterrupts = SunxiGpioQueryEnabledInterrupts;

    //
    // Handlers for GPIO I/O operations.
    //

    RegistrationPacket.CLIENT_ConnectIoPins = SunxiGpioConnectIoPins;
    RegistrationPacket.CLIENT_DisconnectIoPins = SunxiGpioDisconnectIoPins;
    RegistrationPacket.CLIENT_ReadGpioPinsUsingMask = SunxiGpioReadGpioPins;
    RegistrationPacket.CLIENT_WriteGpioPinsUsingMask = SunxiGpioWriteGpioPins;

    //
    // Handlers for GPIO save and restore context (if F-state power mgmt is
    // supported).
    //

#ifdef ENABLE_F_STATE_POWER_MGMT

    RegistrationPacket.CLIENT_SaveBankHardwareContext = SunxiGpioSaveBankHardwareContext;
    RegistrationPacket.CLIENT_RestoreBankHardwareContext = SunxiGpioRestoreBankHardwareContext;

#endif

    //
    // Register the sunxigpio client driver with the GPIO class extension.
    //

    Status = GPIO_CLX_RegisterClient(Driver, &RegistrationPacket, RegistryPath);

DriverEntryEnd:
    return Status;
}

VOID
SunxiGpioEvtDriverUnload (
    _In_ WDFDRIVER Driver
    )

/*++

Routine Description:

    This routine is called by WDF to allow final cleanup prior to unloading the
    sunxigpio client driver. This routine unregisters the client driver from the
    class extension.

Arguments:

    Driver - Supplies a handle to a framework driver object.

Return Value:

    None.

--*/

{

    NTSTATUS Status;

	FunctionEnter();

    PAGED_CODE();

    Status = GPIO_CLX_UnregisterClient(Driver);
    NT_ASSERT(NT_SUCCESS(Status));
}

NTSTATUS
SunxiGpioEvtDeviceAdd (
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This routine is the AddDevice entry point for the client driver. This
    routine is called by the framework in response to AddDevice call from the
    PnP manager. It will create and initialize the device object to represent
    a new instance of the simulated GPIO controller.

Arguments:

    Driver - Supplies a handle to the driver object created in DriverEntry.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS code.

--*/

{
	
	NTSTATUS Status;
    WDFDEVICE Device;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;

	FunctionEnter();

    PAGED_CODE();

    //
    // Call the GPIO class extension's pre-device create interface.
    //

    Status = GPIO_CLX_ProcessAddDevicePreDeviceCreate(Driver,
                                                       DeviceInit,
                                                       &FdoAttributes);
    if (!NT_SUCCESS(Status)) {
        goto EvtDeviceAddEnd;
    }

    //
    // Call the framework to create the device and attach it to the lower stack.
    //

    Status = WdfDeviceCreate(&DeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) {
        goto EvtDeviceAddEnd;
    }

	WdfControlFinishInitializing(Device);

    //
    // Call the GPIO class extension's post-device create interface.
    //

    Status = GPIO_CLX_ProcessAddDevicePostDeviceCreate(Driver, Device);
    if (!NT_SUCCESS(Status)) {
        goto EvtDeviceAddEnd;
    }

EvtDeviceAddEnd:
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
sunxigpiopUnmapControllerBase (
    _In_ PSUNXI_GPIO_CONTEXT GpioContext
    )

/*++

Routine Description:

    This routine releases the memory mapping for the GPIO controller's
    registers, if one has been established.

    N.B. This function is not marked pageable because this function is in
         the device power down path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

Return Value:

    NTSTATUS code.

--*/

{
	FunctionEnter();
    if ((GpioContext->ControllerBase != NULL) &&
        (GpioContext->ControllerBase != GlobalGpioRegisters)) {
        MmUnmapIoSpace(GpioContext->ControllerBase, GpioContext->AddressLength);
        GpioContext->ControllerBase = NULL;
    }
}
//
//------------------------------------------------------------get acpi data
//

NTSTATUS
SendMyIrp(
	IN PDEVICE_OBJECT   Pdo,
	IN ULONG            Ioctl,
	IN PVOID            InputBuffer,
	IN ULONG            InputSize,
	IN PVOID            OutputBuffer,
	IN ULONG            OutputSize
)
/*
Routine Description:
General-purpose function called to send a request to the PDO.
The IOCTL argument accepts the control method being passed down
by the calling function

This subroutine is only valid for the IOCTLS other than ASYNC EVAL.

Parameters:
Pdo             - the request is sent to this device object
Ioctl           - the request - specified by the calling function
InputBuffer     - incoming request
InputSize       - size of the incoming request
OutputBuffer    - the answer
OutputSize      - size of the answer buffer

Return Value:
NT Status of the operation
*/
{
	IO_STATUS_BLOCK     ioBlock;
	KEVENT              myIoctlEvent;
	NTSTATUS            status;
	PIRP                irp;

	
	// Initialize an event to wait on
	KeInitializeEvent(&myIoctlEvent, SynchronizationEvent, FALSE);

	// Build the request
	irp = IoBuildDeviceIoControlRequest(
		Ioctl,
		Pdo,
		InputBuffer,
		InputSize,
		OutputBuffer,
		OutputSize,
		FALSE,
		&myIoctlEvent,
		&ioBlock);

	if (!irp) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Pass request to Pdo, always wait for completion routine
	status = IoCallDriver(Pdo, irp);

	if (status == STATUS_PENDING) {
		// Wait for the IRP to be completed, and then return the status code
		KeWaitForSingleObject(
			&myIoctlEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);

		status = ioBlock.Status;
	}

	return status;
}

NTSTATUS
GetAcpiData(
	_In_ WDFDEVICE Device,
	_In_ PVOID Context
)
{
	PSUNXI_GPIO_CONTEXT			GpioContext;
	ACPI_EVAL_INPUT_BUFFER		inputData = { 0 };
	PACPI_EVAL_OUTPUT_BUFFER	acpiData = NULL;
	PACPI_DATA					outData;
	NTSTATUS					status;
	UCHAR						num1,num2;
	
	FunctionEnter();
	GpioContext = (PSUNXI_GPIO_CONTEXT) Context;
	

		acpiData = ExAllocatePoolWithTag(NonPagedPool, 128, MP_TAG_GENERAL);
		inputData.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
		inputData.MethodNameAsUlong = (ULONG) 'IDOI';
		status = SendMyIrp(
			WdfDeviceWdmGetPhysicalDevice(Device),
			IOCTL_ACPI_EVAL_METHOD,
			&inputData,
			sizeof(ACPI_EVAL_INPUT_BUFFER),
			acpiData,
			128
			);
#if 0
		if((acpiData->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)\
			&&(acpiData->Count==1)\
			&&(acpiData->Argument->Type==ACPI_METHOD_ARGUMENT_BUFFER)\
			&&(acpiData->Argument->DataLength<=128)){
			return;
		}

	    outData = (PACPI_DATA)acpiData->Argument->Data;
#endif
		outData = (PACPI_DATA) acpiData;
		GpioContext->AddressLength = outData->data[0] & 0x0000ffff;
		GpioContext->TotalBanks = (outData->data[0] >> 16) & 0xffff;
		num2 = 1;
		if (GpioContext->TotalBanks < (SUNXI_GPIO_TOTAL_BANKS + 1)) {
			for (num1 = 0; num1 < GpioContext->TotalBanks; num1 += 2) {
				GpioContext->Banks[num1].IsIntBank = (outData->data[num2] & 0x0000ff00) >> 8;
				GpioContext->Banks[num1].NumberOfPins = outData->data[num2] & 0x000000ff;

				GpioContext->Banks[num1 + 1].IsIntBank = (outData->data[num2] & 0xff000000) >> 24;
				GpioContext->Banks[num1 + 1].NumberOfPins = (outData->data[num2] & 0x00ff0000) >> 16;
				num2 += 1;
			}
		}
	

	return status;
}
    
//
// ---------------------------------------------------------- General intefaces
//

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioPrepareController (
    _In_ WDFDEVICE Device,
    _In_ PVOID Context,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )

/*++

Routine Description:

    This routine is called by the GPIO class extension to prepare the
    simulated GPIO controller for use.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ResourcesRaw - Supplies a handle to a collection of framework resource
        objects. This collection identifies the raw (bus-relative) hardware
        resources that have been assigned to the device.

    ResourcesTranslated - Supplies a handle to a collection of framework
        resource objects. This collection identifies the translated
        (system-physical) hardware resources that have been assigned to the
        device. The resources appear from the CPU's point of view.

Return Value:

    NTSTATUS code.

--*/

{
    ULONG Index;
    ULONG InterruptResourceCount;
    ULONG MemoryResourceCount;
    ULONG ResourceCount;
    NTSTATUS Status;
	PSUNXI_GPIO_BANK GpioBank;
	PSUNXI_GPIO_CONTEXT GpioContext;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    UNREFERENCED_PARAMETER(ResourcesRaw);

	FunctionEnter();

    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    RtlZeroMemory(GpioContext, sizeof(SUNXI_GPIO_CONTEXT));

	GetAcpiData(Device, GpioContext);
	
	for (Index = 0; Index < GpioContext->TotalBanks; Index++)
		GpioContext->TotalPins += GpioContext->Banks[Index].NumberOfPins;

	GpioBank = &GpioContext->Banks[0];
    //
    // Walk through the resource list and map all the resources. Atleast one
    // memory resource and one interrupt resource is expected. The resources
    // are described in the ACPI namespace.
    //

    InterruptResourceCount = 0;
    MemoryResourceCount = 0;
    ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    Status = STATUS_SUCCESS;
    for (Index = 0; Index < ResourceCount; Index += 1) {
        Descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, Index);
        switch(Descriptor->Type) {

        //
        // The memory resource supplies the physical register base for the GPIO
        // controller. Map it virtually as non-cached.
        //

        case CmResourceTypeMemory:

            if (MemoryResourceCount == 0) {           	
                GpioContext->PhysicalBaseAddress = Descriptor->u.Memory.Start;
                GpioContext->ControllerBase =
                    (PSUNXI_GPIO_REGISTERS)MmMapIoSpace(Descriptor->u.Memory.Start,
                                                      GpioContext->AddressLength,
                                                      MmNonCached);

				GpioBank->Registers = Add2Ptr(GpioContext->ControllerBase, 0);

                //
                // Fail initialization if mapping of the memory region failed.
                //
                if (GpioContext->ControllerBase == NULL) {
                    Status = STATUS_UNSUCCESSFUL;
                }
			}
			else
			{
				GpioBank->IntLength = sizeof(SUNXI_GPIO_INT_REGISTERS);
				GpioBank->IsIntBank = TRUE;

				GpioBank->IntRegisters = (PSUNXI_GPIO_INT_REGISTERS)MmMapIoSpace(Descriptor->u.Memory.Start,
					GpioBank->IntLength,
					MmNonCached);
				//
				// Fail initialization if mapping of the memory region failed.
				//
				if (GpioBank->IntRegisters == NULL) {
					Status = STATUS_UNSUCCESSFUL;
				}

			}

            MemoryResourceCount += 1;
            break;

        //
        // IO port resources are unexpected, fail initialization.
        //

        case CmResourceTypePort:
            Status = STATUS_UNSUCCESSFUL;
            break;

        case CmResourceTypeInterrupt:
            InterruptResourceCount += 1;
			GpioBank->InterruptResourceIndex = Index;
            break;

        default:
            break;
        }

        if (!NT_SUCCESS(Status)) {
			DbgPrint_E("translate hardware source error!!!");
            goto PrepareControllerEnd;
        }
    }

    if (MemoryResourceCount < 1) {
		DbgPrint_E("this is not memory resource!!!");
        Status = STATUS_UNSUCCESSFUL;
        goto PrepareControllerEnd;
    }

PrepareControllerEnd:
    if (!NT_SUCCESS(Status)) {
		DbgPrint_E("we can not prepare controller properly!");
        sunxigpiopUnmapControllerBase(GpioContext);
    }

	FunctionExit(Status);
    return Status;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
sunxigpioReleaseController (
    _In_ WDFDEVICE Device,
    _In_ PVOID Context
    )

/*++

Routine Description:

    This routine is called by the GPIO class extension to uninitialize the GPIO
    controller.

    N.B. This function is not marked pageable because this function is in
         the device power down path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

Return Value:

    NTSTATUS code.

--*/

{

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Context);

	FunctionEnter();
    //
    // Release the mappings established in the initialize callback.
    //
    // N.B. Disconnecting of the interrupt is handled by the GPIO class
    //      extension.
    //

    sunxigpiopUnmapControllerBase((PSUNXI_GPIO_CONTEXT)Context);
    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioQueryControllerBasicInformation (
    _In_ PVOID Context,
    _Out_ PCLIENT_CONTROLLER_BASIC_INFORMATION ControllerInformation
    )

/*++

Routine Description:

    This routine returns the GPIO controller's attributes to the class extension.

    N.B. This function is not marked pageable because this function is in
         the device power up path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ControllerInformation - Supplies a pointer to a buffer that receives
        controller's information.

Return Value:

    NTSTATUS code.

--*/

{

    PSUNXI_GPIO_CONTEXT GpioContext;

	FunctionEnter();

    ControllerInformation->Version = GPIO_CONTROLLER_BASIC_INFORMATION_VERSION;
    ControllerInformation->Size = sizeof(CLIENT_CONTROLLER_BASIC_INFORMATION);

    //
    // Specify the number of pins on the sunxigpio controller.
    //

    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    ControllerInformation->TotalPins = GpioContext->TotalPins;
    ControllerInformation->NumberOfPinsPerBank = (UCHAR)GpioContext->TotalPins;

    //
    // Indicate that the GPIO controller is memory-mapped and thus can be
    // manipulated at DIRQL.
    //
    // N.B. If the GPIO controller is off-SOC behind some serial bus like
    //      I2C or SPI, then this field must be set to FALSE.
    //

    ControllerInformation->Flags.MemoryMappedController = TRUE;

    //
    // Indicate that status register must be cleared explicitly.
    //

    ControllerInformation->Flags.ActiveInterruptsAutoClearOnRead = 0;

    //
    // Indicate that the client driver would like to receive IO requests as a
    // set of bitmasks as that maps directly to the register operations.
    //

    ControllerInformation->Flags.FormatIoRequestsAsMasks = 1;

    //
    // Indicate that the GPIO controller does not support controller-level
    // D-state power management.
    //

    ControllerInformation->Flags.DeviceIdlePowerMgmtSupported = FALSE;

    //
    // Note if bank-level F-state power management is supported, then specify
    // as such. Note the QuerySetControllerInformation() handler also needs to
    // be implemented in this case.
    //

#ifdef ENABLE_F_STATE_POWER_MGMT

    ControllerInformation->Flags.BankIdlePowerMgmtSupported = TRUE;

#else

    ControllerInformation->Flags.BankIdlePowerMgmtSupported = FALSE;

#endif

    //
    // Note the IdleTimeout parameter does not need to be initialized if
    // D-state power management is not supported.
    //
    // ControllerInformation->IdleTimeout = IdleTimeoutDefaultValue;
    //

    //
    // Note if the GPIO controller does not support hardware debouncing and
    // software-debouncing should be used instead, set the EmulateDebouncing
    // flag.
    //
    // ControllerInformation->Flags.EmulateDebouncing = TRUE;
    //

    //
    // Indicate that the client driver prefers GPIO class extension ActiveBoth
    // emulation.
    //

    ControllerInformation->Flags.EmulateActiveBoth = TRUE;
	FunctionExit(STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioQuerySetControllerInformation (
    _In_ PVOID Context,
    _In_ PCLIENT_CONTROLLER_QUERY_SET_INFORMATION_INPUT InputBuffer,
    _Out_opt_ PCLIENT_CONTROLLER_QUERY_SET_INFORMATION_OUTPUT OutputBuffer
    )

/*++

Routine Description:

    This routine is the generic GPIO query/set handler. Currently it only
    supports returning bank power information.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    InputBuffer - Supplies a pointer to a buffer that receives the parameters
        for the query or set operation.

    OutputBuffer - Supplies a pointer to the GPIO class extension allocated
        buffer to return the output values. Note on entry, the
        OutputBuffer->Size indicates how big the output buffer is. On exit, the
        OutputBuffer->Size indicates the filled-in size or required size.

Return Value:

    NTSTATUS code.

--*/

{

    PPO_FX_COMPONENT_IDLE_STATE F1Parameters;  
	PSUNXI_GPIO_CONTEXT			GpioContext;
	PULONG						ResourceMapping;
	USHORT						TotalBanks;
	USHORT						RequiredOutputSize;
	UCHAR						Index;
	NTSTATUS					Status;

	FunctionEnter();

	GpioContext = (PSUNXI_GPIO_CONTEXT) Context;

    if((InputBuffer == NULL) || (OutputBuffer == NULL)) {
        Status = STATUS_NOT_SUPPORTED;
        goto QuerySetControllerInformationEnd;
    }

	switch (InputBuffer->RequestType)
	{
	case QueryBankPowerInformation:

		//
		// Set the version and size of the output buffer.
		//

		OutputBuffer->Version = GPIO_BANK_POWER_INFORMATION_OUTPUT_VERSION;
		OutputBuffer->Size = sizeof(CLIENT_CONTROLLER_QUERY_SET_INFORMATION_OUTPUT);

		//
		// Mark the given bank (InputBuffer->BankPowerInformation.BankId) as
		// supporting F1 state. Since all banks support it, the BankId is not
		// checked.
		//

		OutputBuffer->BankPowerInformation.F1StateSupported = FALSE;

		//
		// Supply the attributes for the F1 power state.
		//

		F1Parameters = &OutputBuffer->BankPowerInformation.F1IdleStateParameters;
		F1Parameters->NominalPower = SUNXI_GPIO_F1_NOMINAL_POWER;
		F1Parameters->ResidencyRequirement =
			WDF_ABS_TIMEOUT_IN_SEC(SUNXI_GPIO_F1_RESIDENCY);

		F1Parameters->TransitionLatency =
			WDF_ABS_TIMEOUT_IN_SEC(SUNXI_GPIO_F1_TRANSITION);

		Status = STATUS_SUCCESS;
		break;
	case QueryBankInterruptBindingInformation:
		TotalBanks = InputBuffer->BankInterruptBinding.TotalBanks;
		RequiredOutputSize =
			FIELD_OFFSET(CLIENT_CONTROLLER_QUERY_SET_INFORMATION_OUTPUT, \
			BankInterruptBinding.ResourceMapping) +
			(USHORT) (TotalBanks * sizeof(ULONG));

		if (OutputBuffer->Size < RequiredOutputSize) {
			OutputBuffer->Size = RequiredOutputSize;
			Status = STATUS_BUFFER_TOO_SMALL;
			goto QuerySetControllerInformationEnd;
		}

		//
		//  Copy over the mappings, which were already created in the InitializeDevice callback
		//  and stored in InterruptIndex. This is to eliminate having to walk the resources
		//  list a second time
		//

		ResourceMapping =
			(PULONG) &OutputBuffer->BankInterruptBinding.ResourceMapping;

		GpioContext = (PSUNXI_GPIO_CONTEXT) Context;

		for (Index = 0; Index < TotalBanks; Index++) {
			ResourceMapping[Index] = GpioContext->Banks[Index].InterruptResourceIndex;
		}

		OutputBuffer->Version = GPIO_BANK_INTERRUPT_BINDING_INFORMATION_OUTPUT_VERSION;
		OutputBuffer->Size = RequiredOutputSize;
		Status = STATUS_SUCCESS;
		break;
	default:
		Status = STATUS_NOT_SUPPORTED;
		break;
	}

QuerySetControllerInformationEnd:
    return Status;
}


_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioStartController (
    _In_ PVOID Context,
    _In_ BOOLEAN RestoreContext,
    _In_ WDF_POWER_DEVICE_STATE PreviousPowerState
    )

/*++

Routine Description:

    This routine starts the simulated GPIO controller. This routine is
    responsible for configuring all the pins to their default modes.

    N.B. This function is not marked pageable because this function is in
         the device power up path. It is called at PASSIVE_IRQL though.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    RestoreContext - Supplies a flag that indicates whether the client driver
        should restore the GPIO controller state to a previously saved state
        or not.

    PreviousPowerState - Supplies the device power state that the device was in
        before this transition to D0.

Return Value:

    NTSTATUS code.

--*/

{

    BANK_ID BankId;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    //ULONG PinValue;
    GPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS RestoreParameters;
   // PSUNXI_GPIO_REGISTERS sunxigpioRegisters;
	//PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

    UNREFERENCED_PARAMETER(PreviousPowerState);

	FunctionEnter();

    //
    // Perform all the steps necessary to start the device.
    //

    //
    // If restore context is FALSE, then this is initial transition into D0
    // power state for this controller. In such case, disable any interrupts
    // that may have been left enabled (e.g. perhaps by FW, previous D0 -> Dx
    // transition etc.) Otherwise, such interrupts could trigger an interrupt
    // storm if they were to assert without any driver being registered to
    // handle such interrupts.
    //
    // If restore context is TRUE, then this is a transition into D0 power
    // state from a lower power Dx state. In such case, restore the context
    // that was present before the controller transitioned into the lower
    // power state.
    //

    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    if (RestoreContext == FALSE) {
		DbgPrint_V("start contorller  with %d banks.", GpioContext->TotalBanks);
        for (BankId = 0; BankId < GpioContext->TotalBanks; BankId += 1) {
            GpioBank = &GpioContext->Banks[BankId];
           // sunxigpioRegisters = GpioBank->Registers;
			//IntGpioRegisters = GpioBank->IntRegisters;
			
            //
            // Disable all interrupts on this bank by clearing the enable
            // register.
            //

           /* PinValue = READ_REGISTER_ULONG(&sunxigpioRegisters->EnableRegister);
            if (PinValue > 0) {
                PinValue = 0;
                WRITE_REGISTER_ULONG(&sunxigpioRegisters->EnableRegister,
                                     PinValue);
            }*/
        }

    } else {

	    
        //
        // Restoring the controller state involves restoring the state of
        // each sunxigpio bank.
        //
		DbgPrint_V("start contorller and restore content with %d banks.", GpioContext->TotalBanks);
        for (BankId = 0; BankId < GpioContext->TotalBanks; BankId += 1) {
            RestoreParameters.BankId = BankId;
            RestoreParameters.State = PreviousPowerState;
            SunxiGpioRestoreBankHardwareContext(Context, &RestoreParameters);
        }
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioStopController (
    _In_ PVOID Context,
    _In_ BOOLEAN SaveContext,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )

/*++

Routine Description:

    This routine stops the GPIO controller. This routine is responsible for
    resetting all the pins to their default modes.

    N.B. This function is not marked pageable because this function is in
         the device power down path.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    SaveContext - Supplies a flag that indicates whether the client driver
        should save the GPIO controller state or not. The state may need
        to be restored when the controller is restarted.

    TargetState - Supplies the device power state which the device will be put
        in once the callback is complete.

Return Value:

    NTSTATUS code.

--*/

{


    BANK_ID BankId;
    PSUNXI_GPIO_CONTEXT GpioContext;
    GPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS SaveParameters;

    UNREFERENCED_PARAMETER(TargetState);
	FunctionEnter();

    //
    // Perform all the steps necessary to stop the device.
    //

    //
    // If save context is FALSE, then this is a final transition into D3/off
    // power state. Hence saving of context is not necessary.
    //
    // If save context is TRUE, then this is a transition into a lower power
    // Dx state. In such case, save the context as it will need to be
    // restored when the device is brought back to D0 (i.e. ON) power state.
    //

    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    if (SaveContext == TRUE) {
        for (BankId = 0; BankId < GpioContext->TotalBanks; BankId += 1) {
            SaveParameters.BankId = BankId;
            SaveParameters.State = TargetState;
            SunxiGpioSaveBankHardwareContext(Context, &SaveParameters);
        }
    }

    return STATUS_SUCCESS;
}

//
// --------------------------------------------------------- Interrupt Handlers
//

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioEnableInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_ENABLE_INTERRUPT_PARAMETERS EnableParameters
    )

/*++

Routine Description:

    This routine configures the supplied pin for interrupt.

    N.B. This routine is called from within a regular thread context (i.e.,
         non-interrupt context) by the class extension. Thus the interrupt lock
         needs to be explicitly acquired for memory-mapped GPIO controllers
         prior to manipulating any device state that is also affected from a
         routine called within the interrupt context.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    EnableParameters - Supplies a pointer to a structure containing enable
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be enabled. The pin
            number is relative to the bank.

        Flags - Supplies flags controlling the enable operation. Currently
            no flags are defined.

        InterruptMode - Supplies the trigger mode (edge or level) configured for
            this interrupt when it was enabled.

        Polarity - Supplies the polarity (active low or active high) configured
            for this interrupt when it was enabled.
            Note: For edge-triggered interrupts, ActiveLow corresponds to the
                falling edge; ActiveHigh corresponds to the rising edge.

        PullConfiguration - Supplies the pin pull-up/pull-down configuration.

        DebouceTimeout - Supplies the debounce timeout to be applied. The
            field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
            as 584). Default value is zero, which implies, no debounce.

        VendorData - Supplies an optional pointer to a buffer containing the
            vendor data supplied in the GPIO descriptor. This field will be
            NULL if no vendor data was supplied. This buffer is read-only.

        VendorDataLength - Supplies the length of the vendor data buffer.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{
    ULONG Index;
	ULONG Value;
    ULONG PinValue;
	NTSTATUS Status;
	BANK_ID BankId;
	PIN_NUMBER PinNumber;
    PIN_NUMBER ShiftBits;
	PSUNXI_GPIO_BANK GpioBank;
	PSUNXI_GPIO_CONTEXT GpioContext;
	PSUNXI_GPIO_REGISTERS GpioRegisters;
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
    //
    // If the polarity is not supported, then bail out. Note the interrupt
    // polarity cannot be InterruptActiveBoth as this sample uses ActiveBoth
    // emulation.
    //

    if ((EnableParameters->Polarity != InterruptActiveHigh) &&
        (EnableParameters->Polarity != InterruptActiveLow)) {
		DbgPrint_E("interrupt config error!\n");
        Status = STATUS_NOT_SUPPORTED;
        goto EnableInterruptEnd;
    }

    BankId = EnableParameters->BankId;
    PinNumber = EnableParameters->PinNumber;
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
	GpioRegisters = GpioBank->Registers;
    IntGpioRegisters = GpioBank->IntRegisters;
    Status = STATUS_SUCCESS;
	Value = 0;

	//
	//Whether the pin can be configured as an interrupt 
	//

	if (GpioBank->IsIntBank != TRUE) {
		DbgPrint_E("the bank Does not support interrupt function******\n");
		goto EnableInterruptEnd;
	}

	//
	//To determine whether a pin is available
	//
	
	if ((PinNumber >= GpioBank->NumberOfPins) || (PinNumber < 0)) {
		DbgPrint_E("******line:%d the pin Does not support \n");
		goto EnableInterruptEnd;
	}

    //
    // The interrupt enable register/bitmap may be manipulated from within the
    // interrupt context. Hence updates to it must be synchronized using the
    // interrupt lock.
    //

    GPIO_CLX_AcquireInterruptLock(Context, BankId);

	//
	//Pin function is set to interrupt£¨set value£º0x06£©
	//
	Index = PinNumber / 8;
	ShiftBits = (PinNumber % 8) *4;
	PinValue = READ_REGISTER_ULONG(&GpioRegisters->PnCfg[Index]);
	PinValue &= ~(0x0f << ShiftBits);
	PinValue |= (0x06 << ShiftBits);
	WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[Index], PinValue);


	//
	//set pull registers
	//
	Index = PinNumber / 16;
	ShiftBits = (PinNumber % 16) * 2;
	PinValue = READ_REGISTER_ULONG(&GpioRegisters->PnPul[Index]);
	PinValue &= ~(0x03 << ShiftBits);
	PinValue |= EnableParameters->PullConfiguration << ShiftBits;
	WRITE_REGISTER_ULONG(&GpioRegisters->PnPul[Index], PinValue);

	//
	// Set the mode register.0:positive edge, 1:Negative edge, 2:High level, 3: low level,
	//  4:double edge(positive and negative);
	// 
	//

    Index = PinNumber / 8;
    ShiftBits = (PinNumber % 8) * 4;
    PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[Index]);
	if (EnableParameters->InterruptMode == LevelSensitive) {
		switch (EnableParameters->Polarity) {
		case InterruptActiveHigh:
			Value = 0x2;
			break;
		case InterruptActiveLow:
			Value = 0x3;
			break;
		}
	}
	else {
		switch (EnableParameters->Polarity) {

		case InterruptActiveHigh:
			Value = 0;
			break;

		case InterruptActiveLow:
			Value = 1;
			break;
		default:
			Value = 0x04;
		}
	}

	PinValue &= ~(0xf << ShiftBits);
	PinValue |= Value << ShiftBits;
    WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[Index], PinValue);

	//
	//According to DebounceTimeout£¬set debounce register.
	//DebounceTimeout > 100 ,Clock set 1(24M);DebounceTimeout < 100,Clock set 0(32K)
	//pre-scale:0(100)£º0; 10(110):1; 20(120):2; 30(130)£º3; 40(140):4; 50(150):5......
	//
	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntDeb);
	if (EnableParameters->DebounceTimeout > 100) {
		Value = 0x1;
		Value |= ((EnableParameters->DebounceTimeout - 100) / 10) << 4;

	}
	else
	{
		PinValue = 0x1;
		PinValue |= (0x7) << 4;
	}

	WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntDeb, PinValue);

	//
	// NOTE: If the GPIO controller supports a separate set of mask registers,
	//       then any stale value must be cleared here. AwGPIO controller
	//       doesn't and hence this step is skipped here.
	//

	//
	// Clear the corresponding status bit first to ignore any stale value.
	//

	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntSta);
	if ((1 << PinNumber) & PinValue) {
		PinValue = (1 << PinNumber);
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntSta, PinValue);
	}

	//
	// Enable the interrupt by setting the bit in the interrupt enable register.
	//

	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
	PinValue |= (1 << PinNumber);
	WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl, PinValue);

    //
    // Release the interrupt lock.
    //

    GPIO_CLX_ReleaseInterruptLock(Context, BankId);

    Status = STATUS_SUCCESS;

EnableInterruptEnd:
    return Status;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioDisableInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_DISABLE_INTERRUPT_PARAMETERS DisableParameters
    )

/*++

Routine Description:

    This routine disables the supplied pin from interrupting.

    This routine is not marked PAGED as it may be called before/after
    the boot device is in D0/D3 if boot device has GPIO dependencies.

    N.B. This routine is called from within a regular thread context (i.e.,
         non-interrupt context) by the class extension. Thus the interrupt lock
         needs to be explicitly acquired prior to manipulating the device state.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    DisableParameters - Supplies a pointer to a structure supplying the
        parameters for disabling the interrupt. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be disabled. The pin
            number is relative to the bank.

        Flags - Supplies flags controlling the disable operation. Currently
            no flags are defined.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{
	ULONG PinValue;
    BANK_ID BankId;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;
	FunctionEnter();

    BankId = DisableParameters->BankId;
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    IntGpioRegisters = GpioBank->IntRegisters;


    //
    // The interrupt enable register may be manipulated from within the
    // interrupt context. Hence updates to it must be synchronized using the
    // interrupt lock.
    //

    GPIO_CLX_AcquireInterruptLock(Context, BankId);

    //
    // Disable the interrupt by clearing the bit in the interrupt enable
    // register.
    //

	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
	if ((1 << DisableParameters->PinNumber) & PinValue)
		PinValue &= ~(1 << DisableParameters->PinNumber);
	else
		PinValue |= (1 << DisableParameters->PinNumber);
	WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl, PinValue);

    GPIO_CLX_ReleaseInterruptLock(Context, BankId);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SunxiGpioMaskInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_MASK_INTERRUPT_PARAMETERS MaskParameters
    )

/*++

Routine Description:

    This routine invokes masks the supplied pin from interrupting.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    MaskParameters - Supplies a pointer to a structure containing mask
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinMask - Supplies a bitmask of pins which should be masked. If a pin
            should be masked, then the corresponding bit is set in the bitmask.

        FailedMask - Supplies a bitmask of pins that failed to be masked. If
            a pin could not be masked, the bit should be set in this field.

            N.B. This should only be done if for non memory-mapped controllers.
                 Memory-mapped controllers are never expected to fail this
                 operation.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension. Hence the lock is not re-acquired here.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[MaskParameters->BankId];
    IntGpioRegisters = GpioBank->IntRegisters;

    //
    // Mask is essentially same as disable for sunxigpio controller. The
    // difference between the routines is that mask callback is called at DIRQL
    // and automatically synchronized with other DIRQL interrupts callbacks.
    //

    PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
    PinValue &= ~(ULONG)(MaskParameters->PinMask);
    WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl, PinValue);

    //
    // Set the bitmask of pins that could not be successfully masked.
    // Since this is a memory-mapped controller, the mask operation always
    // succeeds.
    //

    MaskParameters->FailedMask = 0x0;
    return STATUS_SUCCESS;
}

NTSTATUS
SunxiGpioUnmaskInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_ENABLE_INTERRUPT_PARAMETERS UnmaskParameters
    )

/*++

Routine Description:

    This routine invokes unmasks the supplied interrupt pin.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    UnmaskParameters - Supplies a pointer to a structure containing parameters
        for unmasking the interrupt. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be unmasked. The pin
            number is relative to the bank.

        InterruptMode - Supplies the trigger mode (edge or level) configured for
            this interrupt when it was enabled.

        Polarity - Supplies the polarity (active low or active high) configured
            for this interrupt when it was enabled.
            Note: For edge-triggered interrupts, ActiveLow corresponds to the
                falling edge; ActiveHigh corresponds to the rising edge.

        PullConfiguration - Supplies the pin pull-up/pull-down configuration.

        DebouceTimeout - Supplies the debounce timeout to be applied. The
            field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
            as 584). Default value is zero, which implies, no debounce.

        VendorData - NULL.

        VendorDataLength - 0.

        N.B. The VendorData and VendorDataLength are not supplied for unmask
             operation (i.e., both fields are zero).

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension. Hence the lock is not re-acquired here.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{
	ULONG PinValue;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[UnmaskParameters->BankId];
    IntGpioRegisters = GpioBank->IntRegisters;

    //
    // Unmask is same as enable on this GPIO controller. The difference between
    // this routine and the enable routine is that unmask callback is called at
    // DIRQL and automatically synchronized with other DIRQL-level callbacks.
    //

    PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
    PinValue |= (1 << UnmaskParameters->PinNumber);
    WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl, PinValue);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SunxiGpioQueryActiveInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_QUERY_ACTIVE_INTERRUPTS_PARAMETERS QueryActiveParameters
    )

/*++

Routine Description:

    This routine returns the current set of active interrupts.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    QueryActiveParameters - Supplies a pointer to a structure containing query
        parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        EnabledMask - Supplies a bitmask of pins enabled for interrupts
            on the specified GPIO bank.

        ActiveMask - Supplies a bitmask that receives the active interrupt
            mask. If a pin is interrupting and set in EnabledMask, then the
            corresponding bit is set in the bitmask.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{
	ULONG PinValue;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
	GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[QueryActiveParameters->BankId];
    IntGpioRegisters = GpioBank->IntRegisters;

    //
    // NOTE: As sunxigpio is not a real hardware device, no interrupt will ever
    //       fire. Thus the status register value will never change. To pretend
    //       as if a real interrupt happened, it marks all currently enabled
    //       interrupts as asserting. Copy the enable interrupt value into
    //       the status register.
    //
    //       This should NOT be done for a real GPIO controller.
    //

    //
    // BEGIN: sunxigpio HACK.
    //

   // PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
   // WRITE_REGISTER_ULONG(&sunxigpioRegisters->StatusRegister, PinValue);

    //
    // END: sunxigpio HACK.
    //

    //
    // Return the current value of the interrupt status register into the
    // ActiveMask parameter.
    //

    PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntSta);
    QueryActiveParameters->ActiveMask = (ULONG64)PinValue;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SunxiGpioQueryEnabledInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_QUERY_ENABLED_INTERRUPTS_PARAMETERS QueryEnabledParameters
    )

/*++

Routine Description:

    This routine returns the current set of enabled interrupts.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    QueryEnabledParameters - Supplies a pointer to a structure containing query
        parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        EnabledMask - Supplies a bitmask that receives the enabled interrupt
            mask. If a pin is enabled, then the corresponding bit is set in the
            mask.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called with the
             interrupt lock acquired by the class extension, but not always
             from within the interrupt context.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{
	ULONG PinValue;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[QueryEnabledParameters->BankId];
    IntGpioRegisters = GpioBank->IntRegisters;

    //
    // Return the current value of the interrupt enable register into the
    // EnabledMask parameter. It is strongly preferred that the true state of
    // the hardware is returned, rather than a software-cached variable, since
    // CLIENT_QueryEnabledInterrupts is used by the class extension to detect
    // interrupt storms.
    //

    PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
    QueryEnabledParameters->EnabledMask = (ULONG64)PinValue;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_same_
NTSTATUS
SunxiGpioClearActiveInterrupts (
    _In_ PVOID Context,
    _In_ PGPIO_CLEAR_ACTIVE_INTERRUPTS_PARAMETERS ClearParameters
    )

/*++

Routine Description:

    This routine clears the GPIO controller's active set of interrupts.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ClearParameters - Supplies a pointer to a structure containing clear
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        ClearActiveMask - Supplies a mask of pins which should be marked as
            inactive. If a pin should be cleared, then the corresponding bit is
            set in the mask.

        FailedMask - Supplies a bitmask of pins that failed to be cleared. If
            a pin could not be cleared, the bit should be set in this field.

            N.B. This should only be done if for non memory-mapped controllers.
                 Memory-mapped controllers are never expected to fail this
                 operation.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{

	ULONG PinValue;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;    
    PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ClearParameters->BankId];
    IntGpioRegisters = GpioBank->IntRegisters;

    //
    // Clear the bits that are set in the ClearActiveMask parameter.
    //

    PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntSta);
    PinValue |= ((ULONG)ClearParameters->ClearActiveMask);
    WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntSta, PinValue);

    //
    // Set the bitmask of pins that could not be successfully cleared.
    // Since this is a memory-mapped controller, the clear operation always
    // succeeds.
    //

    ClearParameters->FailedClearMask = 0x0;
    return STATUS_SUCCESS;
}

NTSTATUS
SunxiGpioReconfigureInterrupt (
    _In_ PVOID Context,
    _In_ PGPIO_RECONFIGURE_INTERRUPTS_PARAMETERS ReconfigureParameters
    )

/*++

Routine Description:

    This routine reconfigures the interrupt in the specified mode.

    N.B. This routine is called with the interrupt lock acquired by the
         class extension. Hence the lock is not re-acquired here.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ReconfigureParameters - Supplies a pointer to a structure containing
        parameters for reconfiguring the interrupt. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumber - Supplies the interrupt line that should be reconfigured.
            The pin number is relative to the bank.

        InterruptMode - Supplies the trigger mode (edge or level) for the new
            configuration.

        Polarity - Supplies the polarity (active low or active high) for the
            new configuration.
            Note: For edge-triggered interrupts, ActiveLow corresponds to the
                falling edge; ActiveHigh corresponds to the rising edge.

Return Value:

    NTSTATUS code.

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped; PASSIVE_LEVEL
        if the controller is behind some serial-bus.

        N.B. For memory-mapped controllers, this routine is called from within
             the interrupt context with the interrupt lock acquired by the class
             extension. Hence the lock is not re-acquired here.

    Synchronization: The GPIO class extension will synchronize this call
        against other query/clear active and enabled interrupts.
        Memory-mapped GPIO controllers:
            Callbacks invoked at PASSIVE_LEVEL IRQL (e.g. interrupt
            enable/disable/unmask or IO operations) may be active. Those
            routines should acquire the interrupt lock prior to manipulating
            any state accessed from within this routine.

        Serial-accessible GPIO controllers:
            This call is synchronized with all other interrupt and IO callbacks.

--*/

{
    ULONG Index;
	ULONG Value;   
    ULONG PinValue;
	BANK_ID BankId;
	NTSTATUS Status;
	PIN_NUMBER PinNumber;
    PIN_NUMBER ShiftBits;
	PSUNXI_GPIO_BANK GpioBank;
	PSUNXI_GPIO_CONTEXT GpioContext;
    PSUNXI_GPIO_REGISTERS GpioRegisters;
	PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;
   

	FunctionEnter();

    BankId = ReconfigureParameters->BankId;
    PinNumber = ReconfigureParameters->PinNumber;
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[BankId];
    GpioRegisters = GpioBank->Registers;
	IntGpioRegisters = GpioBank->IntRegisters;
    Status = STATUS_SUCCESS;
	Value = 0;

	if (GpioBank->IsIntBank != TRUE) {
		DbgPrint_E("the bank Does not support interrupt function");
		return Status;
	}

	//
	//To determine whether a pin is available
	//

	if ((PinNumber >= GpioBank->NumberOfPins) || (PinNumber < 0)) {
		DbgPrint_E("the pin Does not support \n");
		return Status;
	}

    //
    // Clear any stale status bits from the previous configuration.
    //

	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntSta);
	PinValue |= (1 << PinNumber);
	WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntSta, PinValue);


	//
	//Pin function is set to interrupt£¨set value£º0x06£©
	//
	Index = PinNumber / 8;
	ShiftBits = (PinNumber % 8) * 4;
	PinValue = READ_REGISTER_ULONG(&GpioRegisters->PnCfg[Index]);
	PinValue &= ~(0xf << ShiftBits);
	PinValue |= (0x06 << ShiftBits);
	WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[Index], PinValue);

    //
    // Set the mode register. If the interrupt is Level then set the bit;
    // otherwise, clear it (edge-triggered).
    //

	Index = PinNumber / 8;
	ShiftBits = (PinNumber % 8) * 4;
	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[Index]);
	if (ReconfigureParameters->InterruptMode == LevelSensitive) {
		switch (ReconfigureParameters->Polarity) {
		case InterruptActiveHigh:
			Value = 0x2;
			break;
		case InterruptActiveLow:
			Value = 0x3;
			break;
		}
	}
	else {
		switch (ReconfigureParameters->Polarity) {

		case InterruptActiveHigh:
			Value = 0;
			break;

		case InterruptActiveLow:
			Value = 1;
			break;
		default:
			Value = 0x04;
		}
	}

	PinValue &= ~(0xf << ShiftBits);
	PinValue |= (Value << ShiftBits);
	WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[Index], PinValue);

	//
	// Enable the interrupt by setting the bit in the interrupt enable register.
	//

	PinValue = READ_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl);
	PinValue |= (1 << PinNumber);
	WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl, PinValue);

    return STATUS_SUCCESS;
}

//
// --------------------------------------------------------------- I/O Handlers
//

VOID PrintConnectParameters(
	_In_ PGPIO_CONNECT_IO_PINS_PARAMETERS ConnectParameters
	)
/*
Routine Description:
	Print structure PGPIO_CONNECT_IO_PINS_PARAMETERS information¡£

Arguments:
	ConnectParameters - Supplies a pointer to a structure supplying the
	parameters for connecting the IO pins. Fields description:

	BankId - Supplies the ID for the GPIO bank.

	PinNumberTable - Supplies an array of pins to be connected for IO. The
	pin numbers are 0-based and relative to the GPIO bank.

	PinCount - Supplies the number of pins in the pin number table.

	ConnectMode - Supplies the mode in which the pins should be configured
	(viz. input or output).

	ConnectFlags - Supplies the flags controlling the IO setup. Currently
	no flags are defined.

	PullConfiguration - Supplies the pin pull-up/pull-down configuration.

	DebouceTimeout - Supplies the debounce timeout to be applied. The
	field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
	as 584). Default value is zero, which implies, no debounce.

	DriveStrength - Supplies the drive strength to be applied. The value
	is in 100th of mA (i.e., 1.21mA will be supplied as 121mA).

	VendorData - Supplies an optional pointer to a buffer containing the
	vendor data supplied in the GPIO descriptor. This field will be
	NULL if no vendor data was supplied. This buffer is read-only.

	VendorDataLength - Supplies the length of the vendor data buffer.

	ConnectFlags - Supplies the flag to be used for connect operation.
	Currently no flags are defined.

	

*/
{
	ULONG Index;
	PIN_NUMBER PinNumber;

	FunctionEnter();

	for (Index = 0; Index < ConnectParameters->PinCount; Index++) {
		PinNumber = ConnectParameters->PinNumberTable[Index];
		DbgPrint_V("PinNumber: %d\n", PinNumber);
	}

}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioConnectIoPins (
    _In_ PVOID Context,
    _In_ PGPIO_CONNECT_IO_PINS_PARAMETERS ConnectParameters
    )

/*++

Routine Description:

    This routine invokes connects the specified pins for IO. The pins can
    be read from if connected for input, or written to if connected for
    output.

    N.B. This routine is called at PASSIVE_LEVEL but is not marked as
         PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ConnectParameters - Supplies a pointer to a structure supplying the
        parameters for connecting the IO pins. Fields description:

        BankId - Supplies the ID for the GPIO bank.

        PinNumberTable - Supplies an array of pins to be connected for IO. The
            pin numbers are 0-based and relative to the GPIO bank.

        PinCount - Supplies the number of pins in the pin number table.

        ConnectMode - Supplies the mode in which the pins should be configured
            (viz. input or output).

        ConnectFlags - Supplies the flags controlling the IO setup. Currently
            no flags are defined.

        PullConfiguration - Supplies the pin pull-up/pull-down configuration.

        DebouceTimeout - Supplies the debounce timeout to be applied. The
            field is in 100th of milli-seconds (i.e., 5.84ms will be supplied
            as 584). Default value is zero, which implies, no debounce.

        DriveStrength - Supplies the drive strength to be applied. The value
            is in 100th of mA (i.e., 1.21mA will be supplied as 121mA).

        VendorData - Supplies an optional pointer to a buffer containing the
            vendor data supplied in the GPIO descriptor. This field will be
            NULL if no vendor data was supplied. This buffer is read-only.

        VendorDataLength - Supplies the length of the vendor data buffer.

        ConnectFlags - Supplies the flag to be used for connect operation.
            Currently no flags are defined.

Return Value:

    NT status code.

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{
	ULONG Value;
	ULONG Index;
	ULONG PinValue;
	ULONG RegisterNumber;
	ULONG RegisterBitNumber;
	NTSTATUS Status;
	PIN_NUMBER PinNumber;
	PPIN_NUMBER PinNumberTable;
	PSUNXI_GPIO_BANK GpioBank;
	PSUNXI_GPIO_CONTEXT GpioContext;
    PSUNXI_GPIO_REGISTERS GpioRegisters;
    

	FunctionEnter();

	PrintConnectParameters(ConnectParameters);

    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ConnectParameters->BankId];
    GpioRegisters = GpioBank->Registers;
    Status = STATUS_SUCCESS;

	PinNumberTable = ConnectParameters->PinNumberTable;
	for (Index = 0; Index < ConnectParameters->PinCount; Index += 1) {

		PinNumber = PinNumberTable[Index];
		
		//
		//To determine whether a pin is available
		//

		if ((PinNumber >= GpioBank->NumberOfPins) || (PinNumber < 0)) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "the pin is not support!\n"));
			return Status;
		}

		//
		// The first step£ºset config control register.
		//Calculate the control register number and bit number
		//
		RegisterNumber = PinNumber / 8;
		RegisterBitNumber = PinNumber % 8;
		PinValue = READ_REGISTER_ULONG(&GpioRegisters->PnCfg[RegisterNumber]);

		switch (ConnectParameters->ConnectMode)
		{
		case ConnectModeInput:
			PinValue &= ~(0xf << (RegisterBitNumber * 4)); /* input:0 */
			break;
		case ConnectModeOutput:
			PinValue &= ~(0xf << (RegisterBitNumber * 4));/* output:1 */
			PinValue |= (1 << (RegisterBitNumber * 4));
			break;
		default:
			PinValue &= ~(0xf << (RegisterBitNumber * 4));
			PinValue |= ((ULONG) ConnectParameters->VendorData << (RegisterBitNumber * 4));
			break;
		}

		WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[RegisterNumber], PinValue);

		//
		//The second step£ºset driving register
		//Calculate the control register number and bit number
		//

		RegisterNumber = PinNumber / 16;
		RegisterBitNumber = PinNumber % 16;
		PinValue = READ_REGISTER_ULONG(&GpioRegisters->PnDrv[RegisterNumber]);
		if (ConnectParameters->ConnectMode == ConnectModeOutput) {
			switch (ConnectParameters->DriveStrength)
			{
			case 25:
				Value = 0x0;
				break;
			case 50:
				Value = 0x1;
				break;
			case 75:
				Value = 0x3;
				break;
			case 100:
				Value = 0x4;
				break;
			default:
				Value = 0x1;
				break;
			}
		}
		else
			Value = 0x1;

		PinValue &= ~(0x03 << (RegisterBitNumber * 2));
		PinValue |= Value << (RegisterBitNumber * 2);
		WRITE_REGISTER_ULONG(&GpioRegisters->PnDrv[RegisterNumber], PinValue);

		//
		//The third step£º set pull register 
		//Calculate the control register number and bit number
		//

		RegisterNumber = PinNumber / 16;
		RegisterBitNumber = PinNumber % 16;
		PinValue = READ_REGISTER_ULONG(&GpioRegisters->PnPul[RegisterNumber]);
		if (ConnectParameters->ConnectMode == ConnectModeOutput)
			Value = ConnectParameters->PullConfiguration ;
		else
			Value = 0x0;
		
		PinValue &= ~(0x03 << (RegisterBitNumber * 2));
		PinValue |= Value << (RegisterBitNumber * 2);
		WRITE_REGISTER_ULONG(&GpioRegisters->PnPul[RegisterNumber], PinValue);


	}
    return Status;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SunxiGpioDisconnectIoPins (
    _In_ PVOID Context,
    _In_ PGPIO_DISCONNECT_IO_PINS_PARAMETERS DisconnectParameters
    )

/*++

Routine Description:

    This routine invokes disconnects the specified IO pins. The pins are
    put back in their original mode.

    N.B. This routine is called at PASSIVE_LEVEL but is not marked as
         PAGED_CODE as it could be executed late in the hibernate or
         early in resume sequence (or the deep-idle sequence).

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    DisconnectParameters - Supplies a pointer to a structure containing
        disconnect operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinNumberTable - Supplies an array of pins to be disconnected. The pin
            numbers are relative to the GPIO bank.

        PinCount - Supplies the number of pins in the pin number table.

        DisconnectMode - Supplies the mode in which the pins are currently
            configured (viz. input or output).

        DisconnectFlags - Supplies the flags controlling the IO setup. Currently
            no flags are defined.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: PASSIVE_LEVEL.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable/
        unmask) and IO callbacks.

--*/

{

	ULONG				RegisterNumber;
	ULONG				RegisterBitNumber;
	ULONG				Index;
	ULONG				PinValue;
	PIN_NUMBER			PinNumber;
	PPIN_NUMBER			PinNumberTable;
	PSUNXI_GPIO_BANK		GpioBank;
	PSUNXI_GPIO_CONTEXT	GpioContext;
	PSUNXI_GPIO_REGISTERS	AwGpioRegisters;

	FunctionEnter();

	//
	// If the pin configuration should be preserved post disconnect, then
	// there is nothing left to do.
	//

	if (DisconnectParameters->DisconnectFlags.PreserveConfiguration == 1) {
		return STATUS_SUCCESS;
	}

	GpioContext = (PSUNXI_GPIO_CONTEXT) Context;
	GpioBank = &GpioContext->Banks[DisconnectParameters->BankId];
	AwGpioRegisters = GpioBank->Registers;

	//
	// Walk through all the supplied pins and disconnect them. On AwGPIO
	// controller, all pins are reset to the default mode (Disable).
	//

	PinNumberTable = DisconnectParameters->PinNumberTable;
	for (Index = 0; Index < DisconnectParameters->PinCount; Index += 1) {

		PinNumber = PinNumberTable[Index];

		if ((PinNumber >= GpioBank->NumberOfPins) || (PinNumber < 0)) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "the pin is not support!\n"));
			return STATUS_SUCCESS;
		}

		//
		// The first step: set control register(default mode:Disable(0x07))
		//Calculate the control register number and bit number
		//

		RegisterNumber = PinNumber / 8;
		RegisterBitNumber = (PinNumber % 8) * 4;
		PinValue = READ_REGISTER_ULONG(&AwGpioRegisters->PnCfg[RegisterNumber]);
		PinValue &= ~(0xf << RegisterBitNumber);
		PinValue |= (0x07 << RegisterBitNumber);
		WRITE_REGISTER_ULONG(&AwGpioRegisters->PnCfg[RegisterNumber], PinValue);

		//
		//The second step£ºset driving register(default mode:level1(0x01))
		//Calculate the control register number and bit number
		//

		RegisterNumber = PinNumber / 16;
		RegisterBitNumber = (PinNumber % 16) * 2;
		PinValue = READ_REGISTER_ULONG(&AwGpioRegisters->PnDrv[RegisterNumber]);
		PinValue &= ~(0x3 << RegisterBitNumber);
		PinValue |= (0x1 << RegisterBitNumber);
		WRITE_REGISTER_ULONG(&AwGpioRegisters->PnDrv[RegisterNumber], PinValue);

		//
		//The third step£º set pull register (defaule mode pull up/down disable(0)) 
		//Calculate the control register number and bit number
		//

		RegisterNumber = PinNumber / 16;
		RegisterBitNumber = (PinNumber % 16) * 2;
		PinValue = READ_REGISTER_ULONG(&AwGpioRegisters->PnPul[RegisterNumber]);
		PinValue &= ~(0x3 << RegisterBitNumber);
		PinValue |= (0x1 << (RegisterBitNumber ));
		WRITE_REGISTER_ULONG(&AwGpioRegisters->PnPul[RegisterNumber], PinValue);

	}
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
SunxiGpioReadGpioPins (
    _In_ PVOID Context,
    _In_ PGPIO_READ_PINS_MASK_PARAMETERS ReadParameters
    )

/*++

Routine Description:

    This routine reads the current values for all the pins.

    As the FormatIoRequestsAsMasks bit was set inside
    sunxigpioQueryControllerInformation(), all this routine needs to do is read
    the level register value and return to the GPIO class extension. It will
    return the right set of bits to the caller.

    N.B. This routine is called at DIRQL for memory-mapped GPIOs and thus not
         marked as PAGED.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    ReadParameters - Supplies a pointer to a structure containing read
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        PinValues - Supplies a pointer to a variable that receives the current
            pin values.

        Flags - Supplies the flag to be used for read operation. Currently
            defined flags are:

            WriteConfiguredPins: If set, the read is being done on a set of
                pin that were configured for write. In such cases, the
                GPIO client driver is expected to read and return the
                output register value.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped;
        PASSIVE_LEVEL if the controller is behind some serial-bus.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable)
        and IO callbacks (connect/disconnect).

--*/

{
	ULONG PinValue;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;  
    PSUNXI_GPIO_REGISTERS sunxigpioRegisters;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[ReadParameters->BankId];
    sunxigpioRegisters = GpioBank->Registers;

    //
    // Read the current level register value. Note the GPIO class may invoke
    // the read routine on write-configured pins. In such case the output
    // register values should be read.
    //
    // N.B. In case of sunxigpio, the LevelRegister holds the value for input
    //      as well as output pins. Thus the same register is read in either
    //      case.
    //

    if (ReadParameters->Flags.WriteConfiguredPins == FALSE) {
        PinValue = READ_REGISTER_ULONG(&sunxigpioRegisters->PnDat);

    } else {
        PinValue = READ_REGISTER_ULONG(&sunxigpioRegisters->PnDat);
    }

    *ReadParameters->PinValues = PinValue;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
SunxiGpioWriteGpioPins (
    _In_ PVOID Context,
    _In_ PGPIO_WRITE_PINS_MASK_PARAMETERS WriteParameters
    )

/*++

Routine Description:

    This routine sets the current values for the specified pins. This call is
    synchronized with the write and connect/disconnect IO calls.

    N.B. This routine is called at DIRQL for memory-mapped GPIOs and thus not
         marked as PAGED.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    WriteParameters - Supplies a pointer to a structure containing write
        operation parameters. Fields are:

        BankId - Supplies the ID for the GPIO bank.

        SetMask - Supplies a mask of pins which should be set (0x1). If a pin
            should be set, then the corresponding bit is set in the mask.
            All bits that are clear in the mask should be left intact.

        ClearMask - Supplies a mask of pins which should be cleared (0x0). If
            a pin should be cleared, then the bit is set in the bitmask. All
            bits that are clear in the mask should be left intact.

        Flags - Supplies the flag controlling the write operation. Currently
            no flags are defined.

Return Value:

    NTSTATUS code (STATUS_SUCCESS always for memory-mapped GPIO controllers).

Environment:

    Entry IRQL: DIRQL if the GPIO controller is memory-mapped;
        PASSIVE_LEVEL if the controller is behind some serial-bus.

    Synchronization: The GPIO class extension will synchronize this call
        against other passive-level interrupt callbacks (e.g. enable/disable)
        and IO callbacks (connect/disconnect).

--*/

{

    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSUNXI_GPIO_REGISTERS sunxigpioRegisters;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[WriteParameters->BankId];
    sunxigpioRegisters = GpioBank->Registers;

    //
    // Read the current level register value.
    //

    PinValue = READ_REGISTER_ULONG(&sunxigpioRegisters->PnDat);

    //
    // Set the bits specified in the set mask and clear the ones specified
    // in the clear mask.
    //

    PinValue |= WriteParameters->SetMask;
    PinValue &= ~WriteParameters->ClearMask;

    //
    // Write the updated value to the register.
    //

    WRITE_REGISTER_ULONG(&sunxigpioRegisters->PnDat, PinValue);

    return STATUS_SUCCESS;
}

//
// ------------------------------------------------------- Power mgmt handlers
//

VOID
SunxiGpioSaveBankHardwareContext (
    _In_ PVOID Context,
    _In_ PGPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS SaveParameters
    )

/*++

Routine Description:

    This routine saves the hardware context for the GPIO controller.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    SaveRestoreParameters - Supplies a pointer to a structure containing
        parameters for the save operation:

        BankId - Supplies the ID for the GPIO bank.

        State - Target F-state the bank will be transitioned into.

        Flags - Supplies flags for the save operation:
            CriticalTransition - TRUE if this is due to a critical transition.

Return Value:

    None.

--*/

{

    PULONG DestinationAddress;
    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    ULONG Index;
    ULONG RegisterCount;
    PSUNXI_GPIO_REGISTERS sunxigpioRegisters;
	PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;
    PULONG SourceAddress;

	FunctionEnter();
    GpioContext = (PSUNXI_GPIO_CONTEXT)Context;
    GpioBank = &GpioContext->Banks[SaveParameters->BankId];
    sunxigpioRegisters = GpioBank->Registers;
	IntGpioRegisters = GpioBank->IntRegisters;
    //
    // Copy the contents of the registers into memory.
    //

    SourceAddress = &sunxigpioRegisters->PnCfg[0];
    DestinationAddress = &GpioBank->SavedContext.PnCfg[0];
    RegisterCount = sizeof(SUNXI_GPIO_REGISTERS) / sizeof(ULONG);
    for (Index = 0; Index < RegisterCount; Index += 1) {
        *DestinationAddress = READ_REGISTER_ULONG(SourceAddress);
        SourceAddress += 1;
        DestinationAddress += 1;
    }

	if (GpioBank->IsIntBank == TRUE) {

		SourceAddress = &IntGpioRegisters->PnIntCfg[0];
		DestinationAddress = &GpioBank->IntSavedContext.PnIntCfg[0];
		RegisterCount = sizeof(SUNXI_GPIO_INT_REGISTERS) / sizeof(ULONG);

		for (Index = 0; Index < RegisterCount; Index += 1) {
			*DestinationAddress = READ_REGISTER_ULONG(SourceAddress);
			SourceAddress += 1;
			DestinationAddress += 1;
		}
	}

    return;
}

VOID
SunxiGpioRestoreBankHardwareContext (
    _In_ PVOID Context,
    _In_ PGPIO_SAVE_RESTORE_BANK_HARDWARE_CONTEXT_PARAMETERS RestoreParameters
    )

/*++

Routine Description:

    This routine saves the hardware context for the GPIO controller.

Arguments:

    Context - Supplies a pointer to the GPIO client driver's device extension.

    SaveRestoreParameters - Supplies a pointer to a structure containing
        parameters for the restore operation:

        BankId - Supplies the ID for the GPIO bank.

        State - Target F-state the bank will be transitioned into.

        Flags - Supplies flags for the save operation:
            CriticalTransition - TRUE if this is due to a critical transition.

Return Value:

    None.

--*/

{

    PSUNXI_GPIO_BANK GpioBank;
    PSUNXI_GPIO_CONTEXT GpioContext;
    ULONG PinValue;
    PSUNXI_GPIO_REGISTERS GpioRegisters;
	PSUNXI_GPIO_INT_REGISTERS IntGpioRegisters;

	FunctionEnter();
	GpioContext = (PSUNXI_GPIO_CONTEXT) Context;
	GpioBank = &GpioContext->Banks[RestoreParameters->BankId];
	GpioRegisters = GpioBank->Registers;
	IntGpioRegisters = GpioBank->IntRegisters;
	//
	// Restore the level register.
	//

	PinValue = GpioBank->SavedContext.PnDat;
	WRITE_REGISTER_ULONG(&GpioRegisters->PnDat, PinValue);

	//
	// Restore the mode, polarity and enable registers.
	//

	PinValue = GpioBank->SavedContext.PnCfg[0];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[0], PinValue);

	PinValue = GpioBank->SavedContext.PnCfg[1];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[1], PinValue);

	PinValue = GpioBank->SavedContext.PnCfg[2];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[2], PinValue);

	PinValue = GpioBank->SavedContext.PnCfg[3];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnCfg[3], PinValue);

	PinValue = GpioBank->SavedContext.PnDrv[0];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnDrv[0], PinValue);

	PinValue = GpioBank->SavedContext.PnDrv[1];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnDrv[1], PinValue);

	PinValue = GpioBank->SavedContext.PnPul[0];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnPul[0], PinValue);

	PinValue = GpioBank->SavedContext.PnPul[1];
	WRITE_REGISTER_ULONG(&GpioRegisters->PnPul[1], PinValue);

	if (GpioBank->IsIntBank == TRUE) {


		PinValue = GpioBank->IntSavedContext.PnIntCfg[0];
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[0], PinValue);

		PinValue = GpioBank->IntSavedContext.PnIntCfg[1];
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[1], PinValue);

		PinValue = GpioBank->IntSavedContext.PnIntCfg[2];
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[2], PinValue);

		PinValue = GpioBank->IntSavedContext.PnIntCfg[3];
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCfg[3], PinValue);

		PinValue = GpioBank->IntSavedContext.PnIntDeb;
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntDeb, PinValue);

		PinValue = GpioBank->IntSavedContext.PnIntCtl;
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntCtl, PinValue);

		PinValue = GpioBank->IntSavedContext.PnIntSta;
		WRITE_REGISTER_ULONG(&IntGpioRegisters->PnIntSta, PinValue);
	}

    return;
}

__pragma(warning(default: 4127))        // conditional expression is a constant


