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

#include <portcls.h>
#include <wdf.h>

//
// Interface definitions
//


// Device interface
// GUID - {1A208D59-8F8C-4c08-8ED9-7A384543965F}
DEFINE_GUID(GUID_I2S_INTERFACE,
	0x1a208d59, 0x8f8c, 0x4c08, 0x8e, 0xd9, 0x7a, 0x38, 0x45, 0x43, 0x96, 0x5f);

// I2S function interface
// GUID - {02B2478C-452C-43ee-826D-BC05A24252ED}
DEFINE_GUID(GUID_I2S_FUNCTION_INTERFACE,
	0x02B2478C, 0x452C, 0x43ee, 0x82, 0x6D, 0xBC, 0x05, 0xA2, 0x42, 0x52, 0xED);

typedef void (*PI2STransmissionCallbackRoutine)(_In_ PVOID pContext,
	_In_ ULONG BytesTransferred, _In_ NTSTATUS TransferStatus);
typedef struct _I2S_TRANSMISSION_CALLBACK
{
	PVOID pCallbackContext;
	PI2STransmissionCallbackRoutine pCallbackRoutine;
} I2S_TRANSMISSION_CALLBACK, *PI2S_TRANSMISSION_CALLBACK;

typedef void (*PI2SPowerDownCompletionCallbackRoutine)(_In_ PVOID pContext);
typedef struct _I2S_POWER_DOWN_COMPLETION_CALLBACK
{
	PVOID pCallbackContext;
	PI2SPowerDownCompletionCallbackRoutine pCallbackRoutine;
} I2S_POWER_DOWN_COMPLETION_CALLBACK, *PI2S_POWER_DOWN_COMPLETION_CALLBACK;

typedef NTSTATUS (*PI2STransmit)(_In_ PVOID pContext, _In_ PMDL pTransferMdl, _In_ ULONG TransferSize, _In_ ULONG NotificationsPerBuffer);
typedef NTSTATUS (*PI2SReceive)(_In_ PVOID pContext, _In_ PMDL pTransferMdl, _In_ ULONG TransferSize, _In_ ULONG NotificationsPerBuffer);
typedef NTSTATUS (*PI2SStopTransfer)(_In_ PVOID pContext, _In_ BOOL IsCapture);
typedef NTSTATUS(*PI2SSetTransmitCallback)(_In_ PVOID pContext, _In_ PI2S_TRANSMISSION_CALLBACK pTransmissionCallback);
typedef NTSTATUS(*PI2SSetReceiveCallback)(_In_ PVOID pContext, _In_ PI2S_TRANSMISSION_CALLBACK pTransmissionCallback);
typedef NTSTATUS (*PI2SSetStreamFormat)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ PWAVEFORMATEXTENSIBLE pWaveFormatExt);
typedef NTSTATUS (*PI2SSetPowerState)(_In_ PVOID pContext, _In_ DEVICE_POWER_STATE PowerState);
typedef NTSTATUS (*PI2SSetPowerDownCompletionCallback)(_In_ PVOID pContext, _In_ PI2S_POWER_DOWN_COMPLETION_CALLBACK pPowerDownCompletionCallback);

typedef struct _I2S_FUNCTION_INTERFACE
{
	INTERFACE InterfaceHeader;
	PI2STransmit I2STransmit;
	PI2SReceive I2SReceive;
	PI2SStopTransfer I2SStopTransfer;
	PI2SSetTransmitCallback I2SSetTransmitCallback;
	PI2SSetReceiveCallback I2SSetReceiveCallback;
	PI2SSetStreamFormat I2SSetStreamFormat;
	PI2SSetPowerState I2SSetPowerState;
	PI2SSetPowerDownCompletionCallback I2SSetPowerDownCompletionCallback;
} I2S_FUNCTION_INTERFACE, *PI2S_FUNCTION_INTERFACE;

//
// Interface implementations
//
#define DMA_ENTRY_NUMBER 2

#define DMA_MAX_TRANSFER_LENGTH_BYTES 4096
#define DMA_MAX_FRAGMENTS 8

void I2SInterfaceReference(_In_ PVOID pContext);
void I2SInterfaceDereference(_In_ PVOID pContext);
NTSTATUS I2STransmit(_In_ PVOID pContext, _In_ PMDL pTransferMdl, _In_ ULONG TransferSize, _In_ ULONG NotificationsPerBuffer);
NTSTATUS I2SReceive(_In_ PVOID pContext, _In_ PMDL pTransferMdl, _In_ ULONG TransferSize, _In_ ULONG NotificationsPerBuffer);
NTSTATUS I2SStopTransfer(_In_ PVOID pContext, _In_ BOOL IsCapture);
NTSTATUS I2SSetTransmitCallback(_In_ PVOID pContext, _In_ PI2S_TRANSMISSION_CALLBACK pTransmissionCallback);
NTSTATUS I2SSetReceiveCallback(_In_ PVOID pContext, _In_ PI2S_TRANSMISSION_CALLBACK pTransmissionCallback);
NTSTATUS I2SSetStreamFormat(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ PWAVEFORMATEXTENSIBLE pWaveFormatExt);
NTSTATUS I2SSetPowerState(_In_ PVOID pContext, _In_ DEVICE_POWER_STATE PowerState);
NTSTATUS I2SSetPowerDownCompletionCallback(_In_ PVOID pContext, _In_ PI2S_POWER_DOWN_COMPLETION_CALLBACK pPowerDownCompletionCallback);