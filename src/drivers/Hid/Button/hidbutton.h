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

#ifndef _HIDBUTTON_H_
#define _HIDBUTTON_H_

#include <wdm.h>
#include <wdf.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <hidport.h>
#include <pshpack1.h>
#include <poppack.h>

#define FIRST_CONCERT_DLY       (0<<24)
#define CHAN                    (0x3)
#define ADC_CHAN_SELECT         (CHAN<<22)
#define LRADC_KEY_MODE          (0)
#define KEY_MODE_SELECT         (LRADC_KEY_MODE<<12)
#define LEVELB_VOL              (0<<4)

#define LRADC_HOLD_EN           (1<<6)

#define LRADC_SAMPLE_32HZ       (3<<2)
#define LRADC_SAMPLE_62HZ       (2<<2)
#define LRADC_SAMPLE_125HZ      (1<<2)
#define LRADC_SAMPLE_250HZ      (0<<2)


#define LRADC_EN                (1<<0)

#define LRADC_ADC1_UP_EN        (1<<12)
#define LRADC_ADC1_DOWN_EN      (1<<9)
#define LRADC_ADC1_DATA_EN      (1<<8)

#define LRADC_ADC0_UP_EN        (1<<4)
#define LRADC_ADC0_DOWN_EN      (1<<1)
#define LRADC_ADC0_DATA_EN      (1<<0)

#define LRADC_ADC1_UPPEND       (1<<12)
#define LRADC_ADC1_DOWNPEND     (1<<9)
#define LRADC_ADC1_DATAPEND     (1<<8)


#define LRADC_ADC0_UPPEND       (1<<4)
#define LRADC_ADC0_DOWNPEND     (1<<1)
#define LRADC_ADC0_DATAPEND     (1<<0)

#define MODE_0V2
#ifdef MODE_0V2
/* standard of key maping
* 0.2V mode
*/
#define REPORT_START_NUM            (2)
#define REPORT_KEY_LOW_LIMIT_COUNT  (1)
#define MAX_CYCLE_COUNTER           (100)
//#define REPORT_REPEAT_KEY_BY_INPUT_CORE
//#define REPORT_REPEAT_KEY_FROM_HW
#define INITIAL_VALUE               (0Xff)

#define REPORT_ID_KEYBOARD		0x07
#define REPORT_ID_CONSUMER		0x41

static unsigned char keypad_mapindex[64] = {
	0, 0, 0, 0, 0, 0, 0, 0,               	/* key 1, 8���� 0-7 */
	1, 1, 1, 1, 1, 1, 1,                 	/* key 2, 7���� 8-14 */
	2, 2, 2, 2, 2, 2, 2,                 	/* key 3, 7���� 15-21 */
	3, 3, 3, 3, 3, 3,                   	/* key 4, 6���� 22-27 */
	4, 4, 4, 4, 4, 4,                   	/* key 5, 6���� 28-33 */
	5, 5, 5, 5, 5, 5,                   	/* key 6, 6���� 34-39 */
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6,           	/* key 7, 10����40-49 */
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7    	/* key 8, 17����50-63 */
};
#endif

#ifdef MODE_0V15
/* 0.15V mode */
static unsigned char keypad_mapindex[64] = {
	0, 0, 0,    			/* key1 */
	1, 1, 1, 1, 1,                     	/* key2 */
	2, 2, 2, 2, 2,
	3, 3, 3, 3,
	4, 4, 4, 4, 4,
	5, 5, 5, 5, 5,
	6, 6, 6, 6, 6,
	7, 7, 7, 7,
	8, 8, 8, 8, 8,
	9, 9, 9, 9, 9,
	10, 10, 10, 10,
	11, 11, 11, 11,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12  	/*key13 */
};
#endif

#ifdef USE_HARDCODED_HID_REPORT_DESCRIPTOR

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

CONST HID_REPORT_DESCRIPTOR G_DefaultReportDescriptor[] = {
	0x05, 0x01,	// USAGE_PAGE (Generic Desktop)
	0x09, 0x06,	// USAGE (Keyboard)
	0xa1, 0x01,	// COLLECTION (Application)
	0x85, REPORT_ID_KEYBOARD,	// REPORT_ID
	0x05, 0x07,	// USAGE_PAGE (Key Codes)
	0x09, 0xe0,	// USAGE(Left Ctrl Key)
	0x09, 0xe2,	// USAGE(Left Alt Key)
	0x09, 0xe3,	// USAGE(Left Windows Key) 
	0x15, 0x00,	// LOGICAL_MINIMUM (0)
	0x25, 0x01,	// LOGICAL_MAXIMUM (1)
	0x75, 0x01,	// REPORT_SIZE (1)
	0x95, 0x06,	// REPORT_COUNT (3)
	0x81, 0x02,	// INPUT (Data,Var,Abs)
	0x95, 0x1a,	// REPORT_COUNT (29)
	0x81, 0x03,	// INPUT (Cnst,Var,Abs)
	0xc0,	// END_COLLECTION

	0x05, 0x0C,	// USAGE_PAGE (Consumer devices)
	0x09, 0x01,	// USAGE (Consumer Control)
	0xa1, 0x01,	// COLLECTION (Application)
	0x85, REPORT_ID_CONSUMER, 	// REPORT_ID
	0x09, 0xe9, 	// USAGE(Volume up)
	0x09, 0xea, 	// USAGE(Volume down)
	0x15, 0x00,	// LOGICAL_MINIMUM (0)
	0x25, 0x01,	// LOGICAL_MAXIMUM (1)
	0x75, 0x01, 	// REPORT_SIZE (1)
	0x95, 0x02, 	// REPORT_COUNT (2)
	0x81, 0x02, 	// INPUT (Data,Var,Abs)
	0x95, 0x1e, 	// REPORT_COUNT (30)
	0x81, 0x03,	// INPUT (Cnst,Var,Abs)
	0xc0,	// END_COLLECTION
};

const HID_DEVICE_ATTRIBUTES G_DefauleDeviceAttributes = {
	sizeof(G_DefauleDeviceAttributes),
	0x1010,
	0x13,
	0x01,
};

const HID_DESCRIPTOR G_DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	HID_REVISION, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(G_DefaultReportDescriptor) }  // total length of report descriptor
};

#endif // end of USE_HARDCODED_HID_REPORT_DESCRIPTOR

typedef struct _Button_REGISTERS {
	ULONG LradcCtrl;
	ULONG LradcIntc;
	ULONG LradcInts;
	ULONG LradcData0;
	ULONG LradcData1;
}HID_BUTTON_REGISTERS, *PHID_BUTTON_REGISTERS;


typedef struct _DEVICE_EXTENSION{
	//
	// Handle back to the WDFDEVICE
	//
	WDFDEVICE				FxDevice;

	WDFINTERRUPT			InterruptObject;

	LARGE_INTEGER			PhysicalBaseAddress;
	PHID_BUTTON_REGISTERS	Registers;

	//
	// WDF Queue for read IOCTLs from hidclass that get satisfied from 
	// USB interrupt endpoint
	//
	WDFQUEUE				InterruptMsgQueue;

	ULONG					KeyVul;
	ULONG					KeyCnt;
	ULONG					RegVul;
	ULONG					CompareBuffer[REPORT_START_NUM];
	ULONG					ScanCode;
	ULONG					TransferCode;
	UCHAR					JudgeFlag;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _HID_INPUT_REPORT {
	BYTE ReportId;
	BYTE State;
	BYTE Reserved0;
	BYTE Reserved1;
	BYTE Reserved2;

	//BYTE ReportIdC;
	//BYTE ConsumerState;
	//BYTE Reserved3;
	//BYTE Reserved4;
	//BYTE Reserved5;
}HID_INPUT_REPORT, *PHID_INPUT_REPORT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceContext)


EVT_WDF_INTERRUPT_ISR						ButtonInterruptIsr;
EVT_WDF_INTERRUPT_DPC						ButtonInterruptDpc;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL HidButtonEvtInternalDeviceControl;


NTSTATUS
HidButtonGetHidDescriptor(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request
);

NTSTATUS
HidButtonGetReportDescriptor(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request
);


NTSTATUS
HidButtonGetDeviceAttributes(
	IN WDFREQUEST Request
);

EVT_WDF_DEVICE_PREPARE_HARDWARE HidButtonEvtDevicePrepareHardware;

EVT_WDF_DEVICE_D0_ENTRY HidButtonEvtDeviceD0Entry;

EVT_WDF_DEVICE_D0_EXIT HidButtonEvtDeviceD0Exit;


#endif//_HIDBUTTON_H_