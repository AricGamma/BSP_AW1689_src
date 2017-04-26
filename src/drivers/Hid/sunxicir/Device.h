#pragma once
#include "Driver.h"
#include "SunxicirInterface.h"

#define CIR_REG_LENGTH 0x54
#define CIR_GPIO_PIN_COUNT 1
#define CIR_DATA_BUFFER_SIZE 64


NTSTATUS CirDeviceCreate(_In_ PWDFDEVICE_INIT DeviceInit);

typedef enum
{
	CIR_MODE_GPIO
}CIR_GPIO_PIN, *PCIR_GPIO_PIN;

typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE FxDevice;

	ULONG SunxicirInterfaceRef;

	volatile ULONG * RegisterBase;

	ULONG RegisterLength;

	WDFINTERRUPT Interrupt;

	BOOL IsReceiving;

	BOOL PulsePrevious;
	
	ULONG DataBuffer[100];
	
	BOOL PulseBuffer[100];

	ULONG DataSize;

	WDFIOTARGET HidInjectorIoTarget;

}DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext);


EVT_WDF_DEVICE_PREPARE_HARDWARE SunxicirEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE SunxicirEvtDeviceReleaseHardware;

EVT_WDF_DEVICE_D0_ENTRY SunxicirEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT SunxicirEvtDeviceD0Exit;