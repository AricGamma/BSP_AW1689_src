/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

//#include "Hiddesc.h"
//#include <initguid.h>
#include <wdm.h>
#include <wdf.h>
//#include <hidport.h>
//#include <ntstrsafe.h>
#include "trace.h"
//#include "ntdef.h"
#include "public.h"
//#define DEBUG_C_TMH
#include "gsl_point_id.h"
#include "Features.h"
#include "Hiddesc.h"
//
// Pool tag for GSL GPIO allocations.
//
//#define GSL_GPIO_POOL_TAG "GSL_TAG"
#define GSL_GPIO_POOL_TAG 0x100
/*
#define DMA_TRANS_LEN 0x20
#define GSL_PAGE_REG 0xf0 
#define MAX_FINGERS 5
#define MAX_CONTACTS 10
#define MAX_NUMBER_HID_INPUT_REPORT 34  */
#define MAX_NUMBER_IO_RESOURCES 2
#define I2C_RESOURCE_INDEX 0
#define GPIO_RESOURCE_INDEX 1

_inline VOID wd_delay(ULONG milli_sec)
{
	LARGE_INTEGER liTime, startTime, currentTime;

	/* Convert milliseconds to 100-nanosecond increments using:
	*
	*     1 ns = 10 ^ -9 sec
	*   100 ns = 10 ^ -7 sec (1 timer interval)
	*     1 ms = 10 ^ -3 sec
	*     1 ms = (1 timer interval) * 10^4
	*/
	liTime.QuadPart = 0;
	milli_sec = milli_sec * 10000;//how many 100ns.
	    
	KeQuerySystemTime(&startTime);
	while(liTime.QuadPart < milli_sec){
		KeQuerySystemTime(&currentTime);
		liTime.QuadPart = currentTime.QuadPart - startTime.QuadPart;
	}    
}

_inline ULONG QuerySystemTime()
{
	LARGE_INTEGER CurTime, Freq;
	CurTime = KeQueryPerformanceCounter(&Freq);
	return (ULONG)((CurTime.QuadPart * 1000) / Freq.QuadPart);
}//ms

typedef struct _TS_CFG_DATA
{
	BYTE offset;
	UINT32 val;
}TS_CFG_DATA, *PTS_CFG_DATA;

typedef struct _DEVICE_EXTENSION {
	WDFQUEUE IdleQueue;
	WDFQUEUE ReportQueue;
	WDFQUEUE IoctlQueue;
	WDFQUEUE CompletionQueue;
	WDFSPINLOCK DpcLock;
	ULONG IoResourceCount;
	WDFIOTARGET I2CIOTarget;
	ULONG InterruptCount;
	LARGE_INTEGER ConnectionIds[MAX_NUMBER_IO_RESOURCES];
	BOOLEAN DpcRerun;
	BOOLEAN DpcInProgress;
	BOOLEAN NoIDVersion;
	ULONG ScanTime;
	LARGE_INTEGER StartTime;
	BOOLEAN bStartTime;
	BOOLEAN ButtonDown;
	POINT_DATA Point_Data[PS_DEEP][MAX_CONTACTS];
	BYTE ContactCount[PS_DEEP];
#ifdef GSL_TIMER
	PVOID          thread_pointer;
	BOOLEAN            terminate_thread;
	KEVENT          request_event;
	KTIMER EsdTimer;
	KDPC EsdDPC;
	UCHAR int_1st[4];
	UCHAR int_2nd[4];
	UCHAR b0_counter;
	UCHAR bc_counter;
	UCHAR i2c_lock;
#endif
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceContext)

EVT_WDF_DEVICE_PREPARE_HARDWARE HidGSLEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE HidGSLEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY HidGSLEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT HidGSLEvtDeviceD0Exit;
//
#define IOCTL_GPIO_WRITE_PINS                                               \
	CTL_CODE(FILE_DEVICE_GPIO, 0x801, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_GPIO_READ_PINS                                               \
	CTL_CODE(FILE_DEVICE_GPIO, 0x801, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
// Function to initialize the device and its callbacks
//
NTSTATUS
SileadTouchCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );
NTSTATUS
GSL_Init_Chip(IN WDFDEVICE    Device);

NTSTATUS
GSL_Load_CFG(IN WDFDEVICE    Device);

NTSTATUS 
GPIORstReset(IN  WDFDEVICE Device);
NTSTATUS GPIOSHUTDOWN_LOW(IN WDFDEVICE Device);
NTSTATUS GPIOSHUTDOWN_HIGH(IN WDFDEVICE Device);

NTSTATUS
GSL_Reset_Chip(IN WDFDEVICE    Device);

NTSTATUS
GSL_Clr_Reg(IN WDFDEVICE    Device);

NTSTATUS
GSL_Check_Memdata(IN WDFDEVICE    Device);

NTSTATUS
GSL_Startup_Chip(IN WDFDEVICE  Device);

#ifdef GSL9XX_VDDIO_1800
//NTSTATUS GSL_READ_VDDIO(IN WDFDEVICE    Device);
NTSTATUS GSL_IO_CONTROL(IN WDFDEVICE    Device);
#endif
/*
_inline VOID wd_delay(ULONG milli_sec)
{
	ULONG delay = milli_sec*(-10) * 1000;
	LARGE_INTEGER interval;
	interval.QuadPart = delay;
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
}*/

_inline VOID fw2buf(UCHAR *buf, const UINT32 *fw)
{
	UINT32 *u32_buf = (UINT32 *)buf;
	*u32_buf = *fw;
}
BOOLEAN GSLDevInterruptIsr(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID);

VOID
GSLDevInterruptDpc(_In_ WDFINTERRUPT Interrupt, _In_ WDFOBJECT AssociatedObject);

NTSTATUS
GSLDevReadBuffer(
IN WDFDEVICE    Device,
_In_ BYTE RegisterAddress,
_Out_writes_(DataLength) PUCHAR Data,
_In_ USHORT DataLength
);

NTSTATUS
GSLDevWriteBuffer(
IN WDFDEVICE    Device,
_In_ BYTE RegisterAddress,
_In_reads_(DataLength) PUCHAR Data,
_In_ ULONG DataLength
);

VOID
HidGSLEvtInternalDeviceControl(
IN WDFQUEUE     Queue,
IN WDFREQUEST   Request,
IN size_t       OutputBufferLength,
IN size_t       InputBufferLength,
IN ULONG        IoControlCode
);
NTSTATUS
HidGSLGetHidDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
);
NTSTATUS
HidGSLGetReportDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
);
NTSTATUS
HidGSLGetDeviceAttributes(
IN WDFREQUEST Request
);
NTSTATUS
GetFeatureReport(
_In_ WDFDEVICE  FxDevice,
_In_ WDFREQUEST FxRequest
);

NTSTATUS
HidGSLGetString(
_In_ WDFDEVICE  FxDevice,
_In_ WDFREQUEST FxRequest
);

//NTSTATUS HidFx2SendIdleNotification(_In_ WDFREQUEST hRequest);

PCHAR
DbgHidInternalIoctlString(
IN ULONG        IoControlCode
);

_Use_decl_annotations_ BOOLEAN
GSLDevInterruptIsr(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID);
VOID
GSLDevInterruptDpc(_In_ WDFINTERRUPT Interrupt, _In_ WDFOBJECT AssociatedObject);


#ifdef GSL_TIMER
VOID GSLDevESDRoutine(_In_ struct _KDPC *Dpc, _In_opt_ PVOID DeferredContext,
	_In_opt_ PVOID SystemArgument1, _In_opt_ PVOID SystemArgument2
	);
VOID ThreadFunc(IN PVOID Context);
#endif

#ifdef WINKEY_WAKEUP_SYSTEM
NTSTATUS
GSL_Enter_Doze(IN WDFDEVICE    Device);

NTSTATUS
GSL_Exit_Doze(IN WDFDEVICE    Device);
#endif