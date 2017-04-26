#pragma once
#define INITGUID


#include <ntddk.h>
#include <wdf.h>

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD SunxicirEvtDriverDeviceAdd;


EXTERN_C_END