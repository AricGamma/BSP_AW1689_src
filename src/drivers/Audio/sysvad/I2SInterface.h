/*++

Module Name:

	I2SInterface.h - I2S interfaces.

Abstract:

	This file contains the definitions of the interfaces provided in I2S.

Environment:

	Kernel-mode Driver Framework

--*/

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
