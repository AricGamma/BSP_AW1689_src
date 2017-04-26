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
// Definitions for the endpoint notification register
//
typedef enum _ENDPOINT_NOTIFICATION_TYPE 
{
	NOTIFICATION_TYPE_JACK_INFO_CHANGE = 0,
	NOTIFICATION_TYPE_MAX
} ENDPOINT_NOTIFICATION_TYPE, *PENDPOINT_NOTIFICATION_TYPE;

typedef enum _ENDPOINT_DEVICE_TYPE
{
	ENDPOINT_SPEAKER_DEVICE = 0,
	ENDPOINT_SPEAKER_HEADPHONE_DEVICE,
	ENDPOINT_HDMI_RENDER_DEVICE,
	ENDPOINT_MIC_IN_DEVICE,
	ENDPOINT_MIC_ARRAY_DEVICE_1,
	ENDPOINT_MIC_ARRAY_DEVICE_2,
	ENDPOINT_MIC_ARRAY_DEVICE_3,
	ENDPOINT_BTH_HFP_SPEAKER_DEVICE,
	ENDPOINT_BTH_HFP_MIC_DEVICE,
	ENDPOINT_CELLULAR_DEVICE,
	ENDPOINT_HANDSET_SPEAKER_DEVICE,
	ENDPOINT_HANDSET_MIC_DEVICE,
	ENDPOINT_SPEAKER_HEADSET_DEVICE,
	ENDPOINT_MIC_HEADSET_DEVICE,
	ENDPOINT_FM_RX_DEVICE,
	ENDPOINT_SPDIF_RENDER_DEVICE,
	ENDPOINT_MAX_DEVICE_TYPE
} ENDPOINT_DEVICE_TYPE, *PENDPOINT_DEVICE_TYPE;

typedef struct _ENDPOINT_NOTIFICATION_HEAD 
{
	ULONG Size;
	ENDPOINT_NOTIFICATION_TYPE NotificationType;
	ENDPOINT_DEVICE_TYPE DeviceType;
} ENDPOINT_NOTIFICATION_HEAD, *PENDPOINT_NOTIFICATION_HEAD;

typedef void (*PNotificationCallbackRoutine)(_In_ PVOID pContext, _In_ PVOID pNotificationInfo);

typedef struct _ENDPOINT_NOTIFICATION
{
	ENDPOINT_NOTIFICATION_HEAD Header;
	PNotificationCallbackRoutine pCallbackRoutine;
	PVOID pCallbackContex;
} ENDPOINT_NOTIFICATION, *PENDPOINT_NOTIFICATION;

//
// Definitions of the jack info change notification
//
typedef enum _JACK_CONNECTION_STATE
{
	JACK_DISCONNECTED = 0,
	JACK_CONNECTED = 1
} JACK_CONNECTION_STATE, *PJACK_CONNECTION_STATE;

typedef struct _JACK_INFO_CHANGE_NOTIFICATION
{
	ENDPOINT_NOTIFICATION_HEAD Header;
	JACK_CONNECTION_STATE ConnectionState;
} JACK_INFO_CHANGE_NOTIFICATION, *PJACK_INFO_CHANGE_NOTIFICATION;

//
// Definitions for the stream processing mode
//
typedef enum _STREAM_PROCESSING_MODE
{
	STREAM_PROCESSINGMODE_NONE = 0x00,
	STREAM_PROCESSINGMODE_DEFAULT = 0x01,
	STREAM_PROCESSINGMODE_RAW = 0x02,
	STREAM_PROCESSINGMODE_COMMUNICATIONS = 0x04,
	STREAM_PROCESSINGMODE_SPEECH = 0x08,
	STREAM_PROCESSINGMODE_NOTIFICATION = 0x10,
	STREAM_PROCESSINGMODE_MEDIA = 0x20,
	STREAM_PROCESSINGMODE_MOVIE = 0x40
} STREAM_PROCESSING_MODE, *PSTREAM_PROCESSING_MODE;

//
// Definitions for the remote access interface
//

// Codec Device Interface
// GUID - {47607f81-0dd3-4a86-ac39-f36aa76d9a76}
DEFINE_GUID(GUID_CODEC_INTERFACE,
	0x47607f81, 0x0dd3, 0x4a86, 0xac, 0x39, 0xf3, 0x6a, 0xa7, 0x6d, 0x9a, 0x76);

// Codec Function interface
// GUID - {a340608a-a4a4-4723-93de-80b5ceef67cd}
DEFINE_GUID(GUID_CODEC_FUNCTION_INTERFACE,
	0xa340608a, 0xa4a4, 0x4723, 0x93, 0xde, 0x80, 0xb5, 0xce, 0xef, 0x67, 0xcd);

typedef void(*PCodecPowerDownCompletionCallbackRoutine)(_In_ PVOID pContext);
typedef struct _CODEC_POWER_UP_COMPLETION_CALLBACK
{
	PVOID pCallbackContext;
	PCodecPowerDownCompletionCallbackRoutine pCallbackRoutine;
} CODEC_POWER_DOWN_COMPLETION_CALLBACK, *PCODEC_POWER_DOWN_COMPLETION_CALLBACK;

typedef NTSTATUS (*PCodecRegisterEndpointNotification)(_In_ PVOID pContext, _In_ PENDPOINT_NOTIFICATION pNotification);
typedef NTSTATUS (*PCodecSetPowerState)(_In_ PVOID pContext, _In_ DEVICE_POWER_STATE PowerState);
typedef NTSTATUS (*PCodecSetPowerDownCompletionCallback)(_In_ PVOID pContext, _In_ PCODEC_POWER_DOWN_COMPLETION_CALLBACK pPowerDownCompletionCallback);
typedef NTSTATUS (*PCodecSetStreamState)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ KSSTATE State);
typedef NTSTATUS (*PCodecSetStreamMode)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Mode);
typedef NTSTATUS (*PCodecSetStreamFormat)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ PWAVEFORMATEXTENSIBLE pWaveFormatExt);
typedef NTSTATUS (*PCodecSetVolume)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _In_ LONG Volume);
typedef NTSTATUS (*PCodecGetVolume)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _Out_ LONG *pVolume);
typedef NTSTATUS (*PCodecSetMute)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _In_ BOOL IsMute);
typedef NTSTATUS (*PCodecGetMute)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _Out_ BOOL *pIsMute);
typedef NTSTATUS (*PCodecGetPeakMeter)(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _Out_ LONG *pPeakMeter);

typedef struct _CODEC_FUNCTION_INTERFACE
{
	INTERFACE InterfaceHeader;
	PCodecRegisterEndpointNotification CodecRegisterEndpointNotification;
	PCodecSetPowerState CodecSetPowerState;
	PCodecSetPowerDownCompletionCallback CodecSetPowerDownCompletionCallback;
	PCodecSetStreamState CodecSetStreamState;
	PCodecSetStreamMode CodecSetStreamMode;
	PCodecSetStreamFormat CodecSetStreamFormat;
	PCodecSetVolume CodecSetVolume;
	PCodecGetVolume CodecGetVolume;
	PCodecSetMute CodecSetMute;
	PCodecGetMute CodecGetMute;
	PCodecGetPeakMeter CodecGetPeakMeter;
} CODEC_FUNCTION_INTERFACE, *PCODEC_FUNCTION_INTERFACE;


//
// Interface implementations
//
void CodecInterfaceReference(_In_ PVOID pContext);
void CodecInterfaceDereference(_In_ PVOID pContext);
NTSTATUS CodecRegisterEndpointNotification(_In_ PVOID pContext, _In_ PENDPOINT_NOTIFICATION pNotification);
NTSTATUS CodecSetPowerState(_In_ PVOID pContext, _In_ DEVICE_POWER_STATE PowerState);
NTSTATUS CodecSetPowerDownCompletionCallback(_In_ PVOID pContext, _In_ PCODEC_POWER_DOWN_COMPLETION_CALLBACK pPowerDownCompletionCallback);
NTSTATUS CodecSetStreamState(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ KSSTATE State);
NTSTATUS CodecSetStreamMode(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Mode);
NTSTATUS CodecSetStreamFormat(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ PWAVEFORMATEXTENSIBLE pWaveFormatExt);
NTSTATUS CodecSetVolume(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _In_ LONG Volume);
NTSTATUS CodecGetVolume(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _Out_ LONG *pVolume);
NTSTATUS CodecSetMute(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _In_ BOOL Mute);
NTSTATUS CodecGetMute(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _Out_ BOOL *pMute);
NTSTATUS CodecGetPeakMeter(_In_ PVOID pContext, _In_ BOOL IsCapture, _In_ ULONG Channel, _Out_ LONG *pPeakMeter);
