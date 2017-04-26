#pragma once
#include "vhfhid.h"

NTSTATUS VhfHidCreateRawPdo(_In_ WDFDEVICE Device);

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT RawPdoEvtDeviceSelfManagedIoInit;
EVT_WDF_IO_QUEUE_IO_WRITE VhfHidEvtIoWriteForRawPdo;
