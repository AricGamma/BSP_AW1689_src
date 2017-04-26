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

#include "internal.h"
#include "driver.h"
#include "device.h"
#include "ntstrsafe.h"

#include "driver.tmh"

NTSTATUS
#pragma prefast(suppress:__WARNING_DRIVER_FUNCTION_TYPE, "thanks, i know this already")
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG driverConfig;
    WDF_OBJECT_ATTRIBUTES driverAttributes;

    WDFDRIVER fxDriver;

    NTSTATUS status;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    FuncEntry(TRACE_FLAG_WDFLOADING);
	AWPRINT("DriverEntry!");

    WDF_DRIVER_CONFIG_INIT(&driverConfig, OnDeviceAdd);
    driverConfig.DriverPoolTag = SI2C_POOL_TAG;

    WDF_OBJECT_ATTRIBUTES_INIT(&driverAttributes);
    driverAttributes.EvtCleanupCallback = OnDriverCleanup;

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &driverAttributes,
        &driverConfig,
        &fxDriver);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_WDFLOADING,
            "Error creating WDF driver object - %!STATUS!", 
            status);

        goto exit;
    }

    Trace(
        TRACE_LEVEL_VERBOSE, 
        TRACE_FLAG_WDFLOADING,
        "Created WDFDRIVER %p",
        fxDriver);

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}

VOID
OnDriverCleanup(
    _In_ WDFOBJECT Object
    )
{
    UNREFERENCED_PARAMETER(Object);

    WPP_CLEANUP(NULL);
}

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
OnDeviceAdd(
    _In_    WDFDRIVER       FxDriver,
    _Inout_ PWDFDEVICE_INIT FxDeviceInit
    )
/*++
 
  Routine Description:

    This routine creates the device object for an SPB 
    controller and the device's child objects.

  Arguments:

    FxDriver - the WDF driver object handle
    FxDeviceInit - information about the PDO that we are loading on

  Return Value:

    Status

--*/
{
    FuncEntry(TRACE_FLAG_WDFLOADING);

    PPBC_DEVICE pDevice;
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(FxDriver);
	AWPRINT("OnDeviceAdd!");

    //
    // Configure DeviceInit structure
    //
    
    status = SpbDeviceInitConfig(FxDeviceInit);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_WDFLOADING,
            "Failed SpbDeviceInitConfig() for WDFDEVICE_INIT %p - %!STATUS!", 
            FxDeviceInit,
            status);

        goto exit;
    }  
        
    //
    // Setup PNP/Power callbacks.
    //

    {
        WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

        pnpCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
        pnpCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
        pnpCallbacks.EvtDeviceD0Entry = OnD0Entry;
        pnpCallbacks.EvtDeviceD0Exit = OnD0Exit;
        pnpCallbacks.EvtDeviceSelfManagedIoInit = OnSelfManagedIoInit;
        pnpCallbacks.EvtDeviceSelfManagedIoCleanup = OnSelfManagedIoCleanup;

        WdfDeviceInitSetPnpPowerEventCallbacks(FxDeviceInit, &pnpCallbacks);
    }

    //
    // Note: The SPB class extension sets a default 
    //       security descriptor to allow access to 
    //       user-mode drivers. This can be overridden 
    //       by calling WdfDeviceInitAssignSDDLString()
    //       with the desired setting. This must be done
    //       after calling SpbDeviceInitConfig() but
    //       before WdfDeviceCreate().
    //
    

    //
    // Create the device.
    //

    {
		WDFDEVICE fxDevice;
		WDF_OBJECT_ATTRIBUTES deviceAttributes;
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, PBC_DEVICE);
        
        status = WdfDeviceCreate(
            &FxDeviceInit, 
            &deviceAttributes,
            &fxDevice);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Failed to create WDFDEVICE from WDFDEVICE_INIT %p - %!STATUS!", 
                FxDeviceInit,
                status);

            goto exit;
        }

		WdfControlFinishInitializing(fxDevice);
        pDevice = GetDeviceContext(fxDevice);
        NT_ASSERT(pDevice != NULL);
        pDevice->FxDevice = fxDevice;

		{
			ACPI_EVAL_INPUT_BUFFER   inputData = { 0 };
			ACPI_EVAL_OUTPUT_BUFFER	 acpiData;
			RtlZeroMemory(&acpiData, sizeof(ACPI_EVAL_OUTPUT_BUFFER));
			inputData.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
			inputData.MethodNameAsUlong = (ULONG) 'MUNB';
			status = SendMyIrp(
				WdfDeviceWdmGetPhysicalDevice(fxDevice),
				IOCTL_ACPI_EVAL_METHOD,
				&inputData,
				sizeof(ACPI_EVAL_INPUT_BUFFER),
				&acpiData,
				sizeof(ACPI_EVAL_OUTPUT_BUFFER)
				);

			pDevice->BusNumber = (ULONG)acpiData.Argument[0].Data[0];
			pDevice->BusSpeed = acpiData.Argument[0].Argument;
			pDevice->BusSpeed = pDevice->BusSpeed >> 8;
			pDevice->BusSpeed = (pDevice->BusSpeed & 0x000000ff) * 100000;
		
			if (!NT_SUCCESS(status)) {
				goto exit;
			}
		}
    }
        
    //
    // Ensure device is disable-able
    //
    
    {
        WDF_DEVICE_STATE deviceState;
        WDF_DEVICE_STATE_INIT(&deviceState);
        
        deviceState.NotDisableable = WdfFalse;
        WdfDeviceSetDeviceState(pDevice->FxDevice, &deviceState);
    }
    
    //
    // Bind a SPB controller object to the device.
    //

    {
        SPB_CONTROLLER_CONFIG spbConfig;
        SPB_CONTROLLER_CONFIG_INIT(&spbConfig);

        //
        // Register for target connect callback.  The driver
        // does not need to respond to target disconnect.
        //

        spbConfig.EvtSpbTargetConnect    = OnTargetConnect;

        //
        // Register for IO callbacks.
        //

        spbConfig.ControllerDispatchType = WdfIoQueueDispatchSequential;
        spbConfig.PowerManaged           = WdfTrue;
        spbConfig.EvtSpbIoRead           = OnRead;
        spbConfig.EvtSpbIoWrite          = OnWrite;
        spbConfig.EvtSpbIoSequence       = OnSequence;
        spbConfig.EvtSpbControllerLock   = OnControllerLock;
        spbConfig.EvtSpbControllerUnlock = OnControllerUnlock;

        status = SpbDeviceInitialize(pDevice->FxDevice, &spbConfig);
       
        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Failed SpbDeviceInitialize() for WDFDEVICE %p - %!STATUS!", 
                pDevice->FxDevice,
                status);

            goto exit;
        }

        //
        // Register for IO other callbacks.
        //

        SpbControllerSetIoOtherCallback(
            pDevice->FxDevice,
            OnOther,
            OnOtherInCallerContext);
    }

    //
    // Set target object attributes.
    //

    {
        WDF_OBJECT_ATTRIBUTES targetAttributes; 
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&targetAttributes, PBC_TARGET);

        SpbControllerSetTargetAttributes(pDevice->FxDevice, &targetAttributes);
    }

    //
    // Set request object attributes.
    //

    {
        WDF_OBJECT_ATTRIBUTES requestAttributes; 
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, PBC_REQUEST);
        
        //
        // NOTE: Be mindful when registering for EvtCleanupCallback or 
        //       EvtDestroyCallback. IO requests arriving in the class
        //       extension, but not presented to the driver (due to
        //       cancellation), will still have their cleanup and destroy 
        //       callbacks invoked.
        //

        SpbControllerSetRequestAttributes(pDevice->FxDevice, &requestAttributes);
    }

    //
    // Create an interrupt object, interrupt spinlock,
    // and register callbacks.
    //

    {            
        //
        // Create the interrupt spinlock.
        //

        WDF_OBJECT_ATTRIBUTES attributes;
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = pDevice->FxDevice;
        
        WDFSPINLOCK interruptLock;

        status = WdfSpinLockCreate(
           &attributes,
           &interruptLock);
        
        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING, 
                "Failed to create interrupt spinlock for WDFDEVICE %p - %!STATUS!", 
                pDevice->FxDevice,
                status);

            goto exit;
        }

        //
        // Create the interrupt object.
        //

        WDF_INTERRUPT_CONFIG interruptConfig;

        WDF_INTERRUPT_CONFIG_INIT(
            &interruptConfig,
            OnInterruptIsr,
            OnInterruptDpc);

        interruptConfig.SpinLock = interruptLock;

        status = WdfInterruptCreate(
            pDevice->FxDevice,
            &interruptConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pDevice->InterruptObject);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING,
                "Failed to create interrupt object for WDFDEVICE %p - %!STATUS!",
                pDevice->FxDevice,
                status);

            goto exit;
        }
    }
    
    //
    // Create the delay timer to stall between transfers.
    //
    {    
        WDF_TIMER_CONFIG      wdfTimerConfig;
        WDF_OBJECT_ATTRIBUTES timerAttributes;

        WDF_TIMER_CONFIG_INIT(&wdfTimerConfig, OnDelayTimerExpired);
        WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
        timerAttributes.ParentObject = pDevice->FxDevice;

        status = WdfTimerCreate(
            &wdfTimerConfig,
            &timerAttributes,
            &(pDevice->DelayTimer)
            );

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR, 
                TRACE_FLAG_WDFLOADING, 
                "Failed to create delay timer for WDFDEVICE %p - %!STATUS!", 
                pDevice->FxDevice,
                status);

            goto exit;
        }
    }

    //
    // Create the spin lock to synchronize access
    // to the controller driver.
    //
    
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = pDevice->FxDevice;

    status = WdfSpinLockCreate(
       &attributes,
       &pDevice->Lock);
    
    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR, 
            TRACE_FLAG_WDFLOADING, 
            "Failed to create device spinlock for WDFDEVICE %p - %!STATUS!", 
            pDevice->FxDevice,
            status);

        goto exit;
    }
    
    //
    // Configure idle settings to use system
    // managed idle timeout.
    //
    {    
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(
            &idleSettings, 
            IdleCannotWakeFromS0);

        //
        // Explicitly set initial idle timeout delay.
        //

        idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;
        idleSettings.IdleTimeout = IDLE_TIMEOUT_MONITOR_ON;

        status = WdfDeviceAssignS0IdleSettings(
            pDevice->FxDevice, 
            &idleSettings);

        if (!NT_SUCCESS(status))
        {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_WDFLOADING,
                "Failed to initalize S0 idle settings for WDFDEVICE %p- %!STATUS!",
                pDevice->FxDevice,
                status);
                
            goto exit;
        }
    }

exit:

    FuncExit(TRACE_FLAG_WDFLOADING);

    return status;
}
