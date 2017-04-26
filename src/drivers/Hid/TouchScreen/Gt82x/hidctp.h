/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

Module Name:

hidctp.h

Abstract:

common header file

Author:


Environment:

kernel mode only

Notes:


Revision History:


--*/
#ifndef _HIDCTP_H_

#define _HIDCTP_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <wdm.h>
#include "usbdi.h"
#include "usbdlib.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>
#include "wdfusb.h"

#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <hidport.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "trace.h"

#define AWPRINT(string)		KdPrintEx((DPFLTR_IHVDRIVER_ID,  DPFLTR_INFO_LEVEL, "SPBTEST: "##string##"\n"))

EVT_WDF_INTERRUPT_ISR					OnInterruptIsr;
EVT_WDF_INTERRUPT_DPC					OnInterruptDpc;

//
// TODO: Define USE_ALTERNATE_HID_REPORT_DESCRIPTOR to expose only one top level collection
// instead of three.
//
// Comment this line out to expose three top level collections for integration into
// system and consumer button handling.
//
// See the report descriptors for details.
//
#define USE_ALTERNATE_HID_REPORT_DESCRIPTOR

#define _DRIVER_NAME_                 "HIDCTP: "
#define POOL_TAG                      (ULONG) 'CTPF'
#define INPUT_REPORT_BYTES            1

#define CONSUMER_CONTROL_REPORT_ID    1
#define SYSTEM_CONTROL_REPORT_ID      2
#define DIP_SWITCHES_REPORT_ID        3
#define SEVEN_SEGMENT_REPORT_ID       4
#define BARGRAPH_REPORT_ID            5

#define CONSUMER_CONTROL_BUTTONS_BIT_MASK   ((UCHAR)0x7f)   // (first 7 bits)
#define SYSTEM_CONTROL_BUTTONS_BIT_MASK     ((UCHAR)0x80)

#define INTERRUPT_ENDPOINT_INDEX     (0)

#define SWICTHPACK_DEBOUNCE_TIME_IN_MS   10 

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

//
// Define the vendor commands supported by device
//
#define HIDFX2_READ_SWITCH_STATE          0xD6
#define HIDFX2_READ_7SEGMENT_DISPLAY      0xD4
#define HIDFX2_READ_BARGRAPH_DISPLAY      0xD7
#define HIDFX2_SET_BARGRAPH_DISPLAY       0xD8
#define HIDFX2_IS_HIGH_SPEED              0xD9
#define HIDFX2_REENUMERATE                0xDA
#define HIDFX2_SET_7SEGMENT_DISPLAY       0xDB



#ifdef USE_HARDCODED_HID_REPORT_DESCRIPTOR

CONST HID_REPORT_DESCRIPTOR G_DefaultReportDescriptor[] = {
	0x05, 0x0d, // USAGE_PAGE (Digitizers)          
	0x09, 0x04, // USAGE (Touch Screen)             
	0xa1, 0x01, // COLLECTION (Application)  
	0x85, 0x01,                         // REPORT_ID (Touch)
	0x09, 0x22,                         // USAGE (Finger)                 
	0xa1, 0x02,                         // COLLECTION (Logical)
	0x09, 0x42,                         // USAGE (Tip Switch)           
	0x15, 0x00,                         // LOGICAL_MINIMUM (0)          
	0x25, 0x01,                         // LOGICAL_MAXIMUM (1)          
	0x75, 0x01,                         // REPORT_SIZE (1)              
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x95, 0x07,                         // REPORT_COUNT (7)  
	0x81, 0x03,                         // INPUT (Cnst,Ary,Abs)
	0x75, 0x08,                         // REPORT_SIZE (8)
	0x09, 0x51,                         // USAGE (Contact Identifier)
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x05, 0x01,                         // USAGE_PAGE (Generic Desk..
	0x26, 0x00, 0x05,                   // LOGICAL_MAXIMUM (1280)         
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, 0x58, 0x02,                   // PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, 0x90, 0x01,                   // PHYSICAL_MAXIMUM (400)
	0x26, 0x20, 0x03,                   // LOGICAL_MAXIMUM (800) 
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION
	0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
	0x09, 0x22,                         // USAGE (Finger)                 
	0xa1, 0x02,                         // COLLECTION (Logical)
	0x09, 0x42,                         // USAGE (Tip Switch)           
	0x15, 0x00,                         // LOGICAL_MINIMUM (0)          
	0x25, 0x01,                         // LOGICAL_MAXIMUM (1)          
	0x75, 0x01,                         // REPORT_SIZE (1)              
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x95, 0x07,                         // REPORT_COUNT (7)  
	0x81, 0x03,                         // INPUT (Cnst,Ary,Abs)
	0x75, 0x08,                         // REPORT_SIZE (8)
	0x09, 0x51,                         // USAGE (Contact Identifier)
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x05, 0x01,                         // USAGE_PAGE (Generic Desk..
	0x26, 0x00, 0x05,                   // LOGICAL_MAXIMUM (1280)         
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, 0x58, 0x02,                   // PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, 0x90, 0x01,                   // PHYSICAL_MAXIMUM (400)
	0x26, 0x20, 0x03,                   // LOGICAL_MAXIMUM (800) 
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION
	0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
	0x09, 0x22,                         // USAGE (Finger)                 
	0xa1, 0x02,                         // COLLECTION (Logical)
	0x09, 0x42,                         // USAGE (Tip Switch)           
	0x15, 0x00,                         // LOGICAL_MINIMUM (0)          
	0x25, 0x01,                         // LOGICAL_MAXIMUM (1)          
	0x75, 0x01,                         // REPORT_SIZE (1)              
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x95, 0x07,                         // REPORT_COUNT (7)  
	0x81, 0x03,                         // INPUT (Cnst,Ary,Abs)
	0x75, 0x08,                         // REPORT_SIZE (8)
	0x09, 0x51,                         // USAGE (Contact Identifier)
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x05, 0x01,                         // USAGE_PAGE (Generic Desk..
	0x26, 0x00, 0x05,                   // LOGICAL_MAXIMUM (1280)         
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, 0x58, 0x02,                   // PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, 0x90, 0x01,                   // PHYSICAL_MAXIMUM (400)
	0x26, 0x20, 0x03,                   // LOGICAL_MAXIMUM (800) 
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION
	0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
	0x09, 0x22,                         // USAGE (Finger)                 
	0xa1, 0x02,                         // COLLECTION (Logical)
	0x09, 0x42,                         // USAGE (Tip Switch)           
	0x15, 0x00,                         // LOGICAL_MINIMUM (0)          
	0x25, 0x01,                         // LOGICAL_MAXIMUM (1)          
	0x75, 0x01,                         // REPORT_SIZE (1)              
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x95, 0x07,                         // REPORT_COUNT (7)  
	0x81, 0x03,                         // INPUT (Cnst,Ary,Abs)
	0x75, 0x08,                         // REPORT_SIZE (8)
	0x09, 0x51,                         // USAGE (Contact Identifier)
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x05, 0x01,                         // USAGE_PAGE (Generic Desk..
	0x26, 0x00, 0x05,                   // LOGICAL_MAXIMUM (1280)         
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, 0x58, 0x02,                   // PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, 0x90, 0x01,                   // PHYSICAL_MAXIMUM (400)
	0x26, 0x20, 0x03,                   // LOGICAL_MAXIMUM (800) 
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION
	0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
	0x09, 0x22,                         // USAGE (Finger)                 
	0xa1, 0x02,                         // COLLECTION (Logical)
	0x09, 0x42,                         // USAGE (Tip Switch)           
	0x15, 0x00,                         // LOGICAL_MINIMUM (0)          
	0x25, 0x01,                         // LOGICAL_MAXIMUM (1)          
	0x75, 0x01,                         // REPORT_SIZE (1)              
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x95, 0x07,                         // REPORT_COUNT (7)  
	0x81, 0x03,                         // INPUT (Cnst,Ary,Abs)
	0x75, 0x08,                         // REPORT_SIZE (8)
	0x09, 0x51,                         // USAGE (Contact Identifier)
	0x95, 0x01,                         // REPORT_COUNT (1)             
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x05, 0x01,                         // USAGE_PAGE (Generic Desk..
	0x26, 0x00, 0x05,                   // LOGICAL_MAXIMUM (1280)         
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, 0x58, 0x02,                   // PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, 0x90, 0x01,                   // PHYSICAL_MAXIMUM (400)
	0x26, 0x20, 0x03,                   // LOGICAL_MAXIMUM (800) 
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION
	0x05, 0x0d,                         //   USAGE_PAGE (Digitizers)
	0x55, 0x0C,                         //   UNIT_EXPONENT (-4)           
	0x66, 0x01, 0x10,                   //   UNIT (Seconds)        
	0x47, 0xff, 0xff, 0x00, 0x00,       //   PHYSICAL_MAXIMUM (65535)
	0x27, 0xff, 0xff, 0x00, 0x00,       //   LOGICAL_MAXIMUM (65535) 
	0x75, 0x10,                         //   REPORT_SIZE (16)             
	0x95, 0x01,                         //   REPORT_COUNT (1) 
	0x09, 0x56,                         //   USAGE (Scan Time)
	0x81, 0x02,                         //   INPUT (Data,Var,Abs) 
	0x09, 0x54,                         //   USAGE (Contact count)
	0x25, 0x7f,                         //   LOGICAL_MAXIMUM (127) 
	0x95, 0x01,                         //   REPORT_COUNT (1)
	0x75, 0x08,                         //   REPORT_SIZE (8)    
	0x81, 0x02,                         //   INPUT (Data,Var,Abs)
	0x85, 0x05, //   REPORT_ID (Feature)              
	0x09, 0x55,                         //   USAGE(Contact Count Maximum)
	0x95, 0x01,                         //   REPORT_COUNT (1)
	0x25, 0x7f,                         //   LOGICAL_MAXIMUM (127)
	0xb1, 0x02,                         //   FEATURE (Data,Var,Abs)
	0x85, 0x44, //   REPORT_ID (Feature)
	0x06, 0x00, 0xff,                   //   USAGE_PAGE (Vendor Defined)  
	0x09, 0xC5,                         //   USAGE (Vendor Usage 0xC5)    
	0x15, 0x00,                         //   LOGICAL_MINIMUM (0)          
	0x26, 0xff, 0x00,                   //   LOGICAL_MAXIMUM (0xff) 
	0x75, 0x08,                         //   REPORT_SIZE (8)             
	0x96, 0x00, 0x01,                   //   REPORT_COUNT (0x100 (256))             
	0xb1, 0x02,                         //   FEATURE (Data,Var,Abs) 
	0xc0, // END_COLLECTION
};

//CONST HID_REPORT_DESCRIPTOR G_DefaultReportDescriptor[] = {
//	0x05, 0x0d,								// USAGE_PAGE (Digitizers)          
//	0x09, 0x04,								// USAGE (Touch Screen)             
//	0xa1, 0x01,								// COLLECTION (Application)  
//		0x85, 0x01,                         // REPORT_ID (Touch)              
//		0x09, 0x22,                         // USAGE (Finger)                 
//		0xa1, 0x02,                         // COLLECTION (Logical)
//	0x09, 0x42,                         // USAGE (Tip Switch)           
//		0x15, 0x00,                         // LOGICAL_MINIMUM (0)          
//		0x25, 0x01,                         // LOGICAL_MAXIMUM (1)          
//		0x75, 0x01,                         // REPORT_SIZE (1)              
//		0x95, 0x01,                         // REPORT_COUNT (1)             
//	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
//		0x95, 0x07,                         // REPORT_COUNT (7)  
//	0x81, 0x03,                         // INPUT (Cnst,Ary,Abs)
//		0x75, 0x08,                         // REPORT_SIZE (8)
//		0x09, 0x51,                         // USAGE (Contact Identifier)
//		0x95, 0x01,                         // REPORT_COUNT (1)             
//	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
//		0x05, 0x01,                         // USAGE_PAGE (Generic Desk..
//		0x26, 0x00, 0x05,                   // LOGICAL_MAXIMUM (1280)         
//		0x75, 0x10,                         // REPORT_SIZE (16)             
//		0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
//		0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
//		0x09, 0x30,                         // USAGE (X)                    
//		0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
//		0x46, 0x40, 0x06,                   // PHYSICAL_MAXIMUM (1600)
//		0x95, 0x01,                         // REPORT_COUNT (1)         
//	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
//		0x46, 0xE8, 0x03,                   // PHYSICAL_MAXIMUM (1000)
//		0x26, 0x20, 0x03,                   // LOGICAL_MAXIMUM (800) 
//		0x09, 0x31,                         // USAGE (Y)                    
//	0x81, 0x02,                         // INPUT (Data,Var,Abs)
//	0xc0,									//     END_COLLECTION
//	0x05, 0x0d,                         //   USAGE_PAGE (Digitizers)
//		0x55, 0x0C,                         //   UNIT_EXPONENT (-4)           
//		0x66, 0x01, 0x10,                   //   UNIT (Seconds)        
//		0x47, 0xff, 0xff, 0x00, 0x00,       //   PHYSICAL_MAXIMUM (65535)
//		0x27, 0xff, 0xff, 0x00, 0x00,       //   LOGICAL_MAXIMUM (65535) 
//		0x75, 0x10,                         //   REPORT_SIZE (16)             
//		0x95, 0x01,                         //   REPORT_COUNT (1) 
//		0x09, 0x56,                         //   USAGE (Scan Time)
//	0x81, 0x02,                         //   INPUT (Data,Var,Abs) 
//		0x09, 0x54,                         //   USAGE (Contact count)
//		0x25, 0x7f,                         //   LOGICAL_MAXIMUM (127) 
//		0x95, 0x01,                         //   REPORT_COUNT (1)
//		0x75, 0x08,                         //   REPORT_SIZE (8)    
//	0x81, 0x02,                         //   INPUT (Data,Var,Abs)
//	0x85, 0x05,								//   REPORT_ID (Feature)              
//		0x09, 0x55,                         //   USAGE(Contact Count Maximum)
//		0x95, 0x01,                         //   REPORT_COUNT (1)
//		0x25, 0x7f,                         //   LOGICAL_MAXIMUM (127)
//	0xb1, 0x02,                         //   FEATURE (Data,Var,Abs)
//	0x85, 0x44,								//   REPORT_ID (Feature)
//	0x06, 0x00, 0xff,                   //   USAGE_PAGE (Vendor Defined)  
//		0x09, 0xC5,                         //   USAGE (Vendor Usage 0xC5)    
//		0x15, 0x00,                         //   LOGICAL_MINIMUM (0)          
//		0x26, 0xff, 0x00,                   //   LOGICAL_MAXIMUM (0xff) 
//		0x75, 0x08,                         //   REPORT_SIZE (8)             
//		0x96, 0x00, 0x01,                   //   REPORT_COUNT (0x100 (256))             
//	0xb1, 0x02,                         //   FEATURE (Data,Var,Abs) 
//	0xc0,									// END_COLLECTION
//};

const HID_DEVICE_ATTRIBUTES G_DefauleDeviceAttributes = {
	sizeof(G_DefauleDeviceAttributes),
	0x09,
	0x813,
	0x7dc,
};

const HID_DESCRIPTOR G_DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0100, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(G_DefaultReportDescriptor) }  // total length of report descriptor
};

#endif // USE_HARDCODED_HID_REPORT_DESCRIPTOR

#include <pshpack1.h>

typedef struct _HIDFX2_INPUT_REPORT {

	BYTE ReportTouchId;			//Report ID for the collection

	BYTE Tip0;					//Tip Switch 
	BYTE ContactId0;			//contact id
	BYTE XLSB0;					//The X axis LSB
	BYTE XMSB0;					//The X axis MSB
	BYTE YLSB0;					//The Y axis LSB				
	BYTE YMSB0;					//The Y axis LSB

	BYTE Tip1;					//Tip Switch 
	BYTE ContactId1;			//contact id
	BYTE XLSB1;					//The X axis LSB
	BYTE XMSB1;					//The X axis MSB
	BYTE YLSB1;					//The Y axis LSB				
	BYTE YMSB1;					//The Y axis LSB

	BYTE Tip2;					//Tip Switch 
	BYTE ContactId2;			//contact id
	BYTE XLSB2;					//The X axis LSB
	BYTE XMSB2;					//The X axis MSB
	BYTE YLSB2;					//The Y axis LSB				
	BYTE YMSB2;					//The Y axis LSB

	BYTE Tip3;					//Tip Switch 
	BYTE ContactId3;			//contact id
	BYTE XLSB3;					//The X axis LSB
	BYTE XMSB3;					//The X axis MSB
	BYTE YLSB3;					//The Y axis LSB				
	BYTE YMSB3;					//The Y axis LSB

	BYTE Tip4;					//Tip Switch 
	BYTE ContactId4;				//contact id
	BYTE XLSB4;					//The X axis LSB
	BYTE XMSB4;					//The X axis MSB
	BYTE YLSB4;					//The Y axis LSB				
	BYTE YMSB4;					//The Y axis LSB

	BYTE ScanTimeLSB;			//The Scan Time LSB			
	BYTE ScanTimeMSB;			//The Scan Time LSB	
	BYTE ContactCount;          //Contact count

}HIDFX2_INPUT_REPORT, *PHIDFX2_INPUT_REPORT;


typedef struct _HIDFX2_FEATURE_REPORT_COUNT {
	//
	//Report ID for the collection
	//
	BYTE ReportId;
	BYTE ContactCountMaximum; //Contact Count Maximum
}HIDFX2_FEATURE_REPORT_COUNT, *PHIDFX2_FEATURE_REPORT_COUNT;
typedef struct _HIDFX2_FEATURE_REPORT_VENDOR {
	//
	//Report ID for the collection
	//
	BYTE ReportVendorId;
	BYTE Vendor[256];
}HIDFX2_FEATURE_REPORT_VENDOR, *PHIDFX2_FEATURE_REPORT_VENDOR;
#include <poppack.h>


typedef struct _DEVICE_EXTENSION{

	//
	// Handle back to the WDFDEVICE
	//

	WDFDEVICE FxDevice;

	//
	// Connection ID for SPB peripheral
	//

	LARGE_INTEGER PeripheralId;

	//wakeup  pin
	LARGE_INTEGER ConnectionIds;

	WDFINTERRUPT Interrupt;

	WDFIOTARGET SpbController;

	//
	// SPB request object
	//

	WDFREQUEST SpbRequest;


	//
	// WDF Queue for read IOCTLs from hidclass that get satisfied from 
	// USB interrupt endpoint
	//
	WDFQUEUE   InterruptMsgQueue;

	BOOLEAN				IsInterruptEnable;
	WDFSPINLOCK         OnLock;
	UCHAR				TouchNum;
	UCHAR				Report;
	BOOLEAN				IsInit;

	BOOLEAN				DataReady;
	UCHAR				Data[30];
	long				ReportTime;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceContext)

//
// driver routine declarations
//
// This type of function declaration is for Prefast for drivers. 
// Because this declaration specifies the function type, PREfast for Drivers
// does not need to infer the type or to report an inference. The declaration
// also prevents PREfast for Drivers from misinterpreting the function type 
// and applying inappropriate rules to the function. For example, PREfast for
// Drivers would not apply rules for completion routines to functions of type
// DRIVER_CANCEL. The preferred way to avoid Warning 28101 is to declare the
// function type explicitly. In the following example, the DriverEntry function
// is declared to be of type DRIVER_INITIALIZE.
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD HidCtpEvtDeviceAdd;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL HidCtpEvtInternalDeviceControl;

NTSTATUS
HidCtpGetHidDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
);

NTSTATUS
HidCtpGetReportDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
);


NTSTATUS
HidCtpGetDeviceAttributes(
IN WDFREQUEST Request
);

EVT_WDF_DEVICE_PREPARE_HARDWARE HidCtpEvtDevicePrepareHardware;

EVT_WDF_DEVICE_D0_ENTRY HidCtpEvtDeviceD0Entry;

EVT_WDF_DEVICE_D0_EXIT HidCtpEvtDeviceD0Exit;

PCHAR
DbgDevicePowerString(
IN WDF_POWER_DEVICE_STATE Type
);

EVT_WDF_OBJECT_CONTEXT_CLEANUP HidCtpEvtDriverContextCleanup;

PCHAR
DbgHidInternalIoctlString(
IN ULONG        IoControlCode
);


NTSTATUS
HidCtpSetFeature(
IN WDFREQUEST Request
);

NTSTATUS
HidCtpGetFeature(
IN WDFREQUEST Request,
OUT PULONG BytesReturned
);

EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE HidCtpEvtIoCanceledOnQueue;


NTSTATUS SpbRequestWrite(
	_In_ PDEVICE_EXTENSION pDevice,
	_In_ PUCHAR	InBuffer,
	_In_ UCHAR	InBufferlength

	);

NTSTATUS SpbRequestRead(
	_In_ PDEVICE_EXTENSION  pDevice,
	_Out_ PUCHAR OutBuffer,
	_In_ UCHAR	OutBufferlength

	);


NTSTATUS SpbRequestWriteRead(
	_In_ PDEVICE_EXTENSION pDevice,
	_In_ PUCHAR	InBuffer,
	_In_ UCHAR	InBufferlength,
	_Out_ PUCHAR OutBuffer,
	_In_ UCHAR	OutBufferlength

	);

NTSTATUS
EndCmd(
_In_ PDEVICE_EXTENSION pDevice
);

NTSTATUS
CtpInit(
_In_ PDEVICE_EXTENSION pDevice
);

VOID
ReadChipId(
_In_ PDEVICE_EXTENSION pDevice
);

NTSTATUS
TestReadWrite(
_In_ WDFDEVICE Device,
_In_ PCUNICODE_STRING RequestString,
_In_ BOOLEAN ReadOperation,
_Inout_ PUCHAR Data,
_In_ _In_range_(>, 0) ULONG Size,
_Out_ WDFIOTARGET *IoTargetOut
);


VOID
Wait(
LONG msec
);

VOID
SetWakeupHigh(
_In_ PDEVICE_EXTENSION pDevice,
LONG msec
);

VOID
GoodixTsVersion(
_In_ PDEVICE_EXTENSION pDevice
);

NTSTATUS
SpbPeripheralOpen(
_In_  PDEVICE_EXTENSION pDevice
);

NTSTATUS
SpbPeripheralClose(
_In_  PDEVICE_EXTENSION  pDevice
);

#endif   //_HIDCTP_H_

