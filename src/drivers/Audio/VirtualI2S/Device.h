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

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include "I2SInterface.h"

//
// Registry configs
//

// Driver configured idle timeout value, not efficient in current design.
// Because we configure the SystemManagedIdleTimeoutWithHint in Idle Settings.
#define IDLE_TIMEOUT_VALUE_NAME L"IdleTimeout"
#define DEFAULT_IDLE_TIMEOUT_VALUE_MS 500

//
// Clock frequency changing configs
//

// {F71A096E-3065-4411-921E-0A509DE2B4C7}
DEFINE_GUID(POFX_POWER_CONTROL_CODE_CLOCK_FREQUENCY_CHANGE,
	0xf71a096e, 0x3065, 0x4411, 0x92, 0x1e, 0xa, 0x50, 0x9d, 0xe2, 0xb4, 0xc7);

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{	
	//
	// I2S interface
	//
	I2S_FUNCTION_INTERFACE I2sFunctionInterface;
	ULONG I2sInterfaceRef;

	WDFDEVICE pDevice;
	WDF_POWER_DEVICE_STATE PowerState;
	I2S_POWER_DOWN_COMPLETION_CALLBACK PowerDownCompletionCallback;
	BOOLEAN IsFirstD0Entry;
	POHANDLE hPoHandle;

	//
	// DMA resources
	//
	PCM_PARTIAL_RESOURCE_DESCRIPTOR pDmaResourceDescriptor[DMA_ENTRY_NUMBER];

	WDFSPINLOCK pDmaTransmissionLock;

	ULONG DmaState;
	WDFDMAENABLER pTxDmaEnabler;
	WDFDMAENABLER pRxDmaEnabler;
	WDFDMATRANSACTION pDmaTransmitTransaction;
	WDFDMATRANSACTION pDmaReceiveTransaction;
	PDMA_ADAPTER pReadAdapter;
	PDMA_ADAPTER pWriteAdapter;
	DMA_ADAPTER_INFO DmaReadInfo;
	DMA_ADAPTER_INFO DmaWriteInfo;
	ULONG DmaTransmitTotalLength;
	ULONG DmaTransmitLengthPerOperation;
	ULONG DmaTransmittedLength;
	ULONG DmaReceiveTotalLength;
	ULONG DmaReceiveLengthPerOperation;
	ULONG DmaReceivedLength;
	PMDL pTransmitMdl;
	PMDL pReceiveMdl;
	ULONG DmaInterruptCounter;
	INT	IdleState;
	ULONG TransferCounter;

	I2S_TRANSMISSION_CALLBACK DmaTransmitCallback;
	I2S_TRANSMISSION_CALLBACK DmaReceiveCallback;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

EXTERN_C_START

EVT_WDF_DEVICE_PREPARE_HARDWARE I2SEvtPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE I2SEvtReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY I2SEvtD0Entry;
EVT_WDF_DEVICE_D0_EXIT I2SEvtD0Exit;
EVT_WDFDEVICE_WDM_POST_PO_FX_REGISTER_DEVICE I2SSingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice;
EVT_WDFDEVICE_WDM_PRE_PO_FX_UNREGISTER_DEVICE I2SSingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice;

NTSTATUS I2SCreateDevice(_In_ PWDFDEVICE_INIT pDeviceInit);

EXTERN_C_END