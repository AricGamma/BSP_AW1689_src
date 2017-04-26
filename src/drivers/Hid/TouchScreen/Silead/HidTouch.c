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

#include "Device.h"
#include "Hiddesc.h"
#include "Idle.h"
#ifdef DEBUG_C_TMH
#include "HidTouch.tmh"
#endif


CONST  HID_REPORT_DESCRIPTOR           G_DefaultReportDescriptor[] = {//386
	0x05, 0x0d, // USAGE_PAGE (Digitizers) 
	0x09, 0x04, // USAGE (Touch Screen)             
	0xa1, 0x01, // COLLECTION (Application)  
	0x85, REPORTID_INPUT,               // REPORT_ID (Touch)
         
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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f1)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f2)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f3)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f4)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f5)
#ifndef	SET_MULTITOUCH_5POINT
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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f6)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f7)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f8)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f9)

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
	0x26, LOGICAL_XLSB, LOGICAL_XMSB,	// LOGICAL_MAXIMUM (1280)
	0x75, 0x10,                         // REPORT_SIZE (16)             
	0x55, 0x0e,                         // UNIT_EXPONENT (-2)           
	0x65, 0x13,                         // UNIT(Inch,EngLinear)                  
	0x09, 0x30,                         // USAGE (X)                    
	0x35, 0x00,                         // PHYSICAL_MINIMUM (0)         
	0x46, PHYSICAL_XLSB, PHYSICAL_XMSB,	// PHYSICAL_MAXIMUM (600)
	0x95, 0x01,                         // REPORT_COUNT (1)         
	0x81, 0x02,                         // INPUT (Data,Var,Abs) 
	0x46, PHYSICAL_YLSB, PHYSICAL_YMSB,	// PHYSICAL_MAXIMUM (400)
	0x26, LOGICAL_YLSB, LOGICAL_YMSB,	 // LOGICAL_MAXIMUM (800)
	0x09, 0x31,                         // USAGE (Y)                    
	0x81, 0x02,                         // INPUT (Data,Var,Abs)
	0xc0, //     END_COLLECTION(f10)
#endif
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
	0x85, REPORTID_MAX_COUNT,           //   REPORT_ID (Feature)              
	0x09, 0x55,                         //   USAGE(Contact Count Maximum)
	0x95, 0x01,                         //   REPORT_COUNT (1)
	0x25, 0x7f,                         //   LOGICAL_MAXIMUM (127)
	0xb1, 0x02,                         //   FEATURE (Data,Var,Abs)
	0x85, REPORTID_FW_KEY,              //   REPORT_ID (Feature)
	0x06, 0x00, 0xff,                   //   USAGE_PAGE (Vendor Defined)  
	0x09, 0xC5,                         //   USAGE (Vendor Usage 0xC5)    
	0x15, 0x00,                         //   LOGICAL_MINIMUM (0)          
	0x26, 0xff, 0x00,                   //   LOGICAL_MAXIMUM (0xff) 
	0x75, 0x08,                         //   REPORT_SIZE (8)             
	0x96, 0x00, 0x01,                   //   REPORT_COUNT (0x100 (256))             
	0xb1, 0x02,                         //   FEATURE (Data,Var,Abs) 
	0xc0, // END_COLLECTION

#ifdef SUPPORT_WINKEY
	0x05, 0x01,	// USAGE_PAGE (Generic Desktop)
	0x09, 0x06,	// USAGE (Keyboard) 
	0xa1, 0x01,	// COLLECTION (Application)
	0x85, REPORTID_WINKEY,	// REPORT_ID
	0x05, 0x07,	//USAGE_PAGE (Keyboard)
	0x19, 0xe0,	//USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,	//USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,	//LOGICAL_MINIMUM (0)
	0x25, 0x01,	//LOGICAL_MAXIMUM (1)
	0x75, 0x01,	//REPORT_SIZE (1)
	0x95, 0x08,	//REPORT_COUNT (8)
	0x81, 0x02,	//INPUT (DataVarAbs)
	0x95, 0x01,	//REPORT_COUNT (1)
	0x75, 0x08,	//REPORT_SIZE (8)

	0x81, 0x03,	//INPUT (CnstVarAbs)
	0x95, 0x05,	//REPORT_COUNT (5)
	0x75, 0x01,	//REPORT_SIZE (1)
	0x05, 0x08,	//USAGE_PAGE (LEDs)
	0x19, 0x01,	//USAGE_MINIMUM (Num Lock)
	0x29, 0x05,	//USAGE_MAXIMUM (Kana)
	0x91, 0x02,	//OUTPUT (DataVarAbs)
	0x95, 0x01,	//REPORT_COUNT (1)
	0x75, 0x03,	//REPORT_SIZE (3)
	0x91, 0x03,	//OUTPUT (CnstVarAbs)

	0x95, 0x06,	//REPORT_COUNT (6)
	0x75, 0x08,	//REPORT_SIZE (8)
	0x15, 0x00,	//LOGICAL_MINIMUM (0)  
	0x05, 0x07,	//USAGE_PAGE (Keyboard)
	0x19, 0x00,	//USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,	//USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,	//INPUT (DataAryAbs)
	0xc0,	// END_COLLECTION  
	//以上定义了6个8bit宽的数组，每个8bit（即一个字节）用来表示一个按键，所以可以同时
	//有6个按键按下。没有按键按下时，全部返回0。如果按下的键太多，导致键盘扫描系统
	//无法区分按键时，则全部返回0x01，即6个0x01。如果有一个键按下，则这6个字节中的第一
	//个字节为相应的键值（具体的值参看HID Usage Tables），如果两个键按下，则第1、2两个
	//字节分别为相应的键值，以次类推。
#endif
};

CONST HID_DESCRIPTOR G_DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0100, // hid spec release,version number,default to 0x100
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(G_DefaultReportDescriptor) }  // total length of report descriptor
};

VOID
HidGSLEvtInternalDeviceControl(
IN WDFQUEUE     Queue,
IN WDFREQUEST   Request,
IN size_t       OutputBufferLength,
IN size_t       InputBufferLength,
IN ULONG        IoControlCode
)
{
	NTSTATUS            status = STATUS_SUCCESS;
	WDFDEVICE           device;
	PDEVICE_EXTENSION   devContext = NULL;
	//ULONG               bytesReturned = 0;
	BOOLEAN requestPending;
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	device = WdfIoQueueGetDevice(Queue);
	devContext = GetDeviceContext(device);

	DbgPrintGSL("%s, Queue:0x%p, Request:0x%p\n",
		DbgHidInternalIoctlString(IoControlCode),
		Queue,
		Request
		);

	switch (IoControlCode) {

	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		status = HidGSLGetHidDescriptor(device, Request);
		break;

	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		status = HidGSLGetDeviceAttributes(Request);
		break;

	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		status = HidGSLGetReportDescriptor(device, Request);
		break;

	case IOCTL_HID_READ_REPORT:
		status = WdfRequestForwardToIoQueue(Request, devContext->ReportQueue);

		if (!NT_SUCCESS(status)){
			DbgPrint("WdfRequestForwardToIoQueue failed with status: 0x%x\n", status);

			WdfRequestComplete(Request, status);
		}
		return;

		/*
		The Parameters.DeviceIoControl.OutputBufferLength member specifies the size,
		in bytes, of a requester-allocated output buffer.
		The HID class driver uses this buffer to return an input report.
		The buffer size, in bytes, must be large enough to hold the input report
		-- excluding its report ID, if report IDs are used -- plus one additional byte that
		specifies a nonzero report ID or zero.
		*//*
	case IOCTL_HID_GET_INPUT_REPORT:///device to host. // change here no need to process this
		status = HidGSLGetInputReport(Request, device);
		return;*/

	case IOCTL_HID_GET_FEATURE: // change here need to process this
		status = GetFeatureReport(device, Request);
		//WdfRequestCompleteWithInformation(Request, status, bytesReturned);
		break;

	case IOCTL_HID_GET_STRING:
		status = HidGSLGetString(device, Request);
		break;
		/*
	case IOCTL_HID_SET_FEATURE:
		status = HidGSLSetFeature(Request);
		WdfRequestComplete(Request, status);
		return;*/
	/*case IOCTL_HID_SET_OUTPUT_REPORT:
		status = HidGSLSetOutputReport(Request);
		return;*/	
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST: //Pass down Idle notification request to lower driver for USB // change here need to process this
		// need to re-wite for correctly handling
		status = TchProcessIdleRequest(device, Request, &requestPending);
		if (requestPending)
			return;
		break;

	case IOCTL_HID_WRITE_REPORT:
	case IOCTL_HID_ACTIVATE_DEVICE:
	case IOCTL_HID_DEACTIVATE_DEVICE:
	case IOCTL_HID_SET_OUTPUT_REPORT:			    //host to device

	case IOCTL_HID_SET_FEATURE:
	case IOCTL_HID_GET_INPUT_REPORT:
	default:
		status = STATUS_NOT_SUPPORTED;
		break;
	}

	WdfRequestComplete(Request, status);

	return;
}


NTSTATUS
HidGSLGetHidDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
)
/*++

Routine Description:

Finds the HID descriptor and copies it into the buffer provided by the
Request.

Arguments:

Device - Handle to WDF Device Object

Request - Handle to request object

Return Value:

NT status code.

--*/
{
	NTSTATUS            status = STATUS_SUCCESS;
	size_t              bytesToCopy = 0;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	DbgPrintGSL("HidGSLGetHidDescriptor Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfRequestRetrieveOutputMemory failed 0x%x\n", status);
		return status;
	}

	//
	// Use hardcoded "HID Descriptor" 
	//
	bytesToCopy = G_DefaultHidDescriptor.bLength;

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		DbgPrint("G_DefaultHidDescriptor is zero, 0x%x\n", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0, // Offset
		(PVOID)&G_DefaultHidDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfMemoryCopyFromBuffer failed 0x%x\n", status);
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	DbgPrintGSL("HidGSLGetHidDescriptor Exit = 0x%x\n", status);
	return status;
}

NTSTATUS
HidGSLGetReportDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request
)
/*++

Routine Description:

Finds the Report descriptor and copies it into the buffer provided by the
Request.

Arguments:

Device - Handle to WDF Device Object

Request - Handle to request object

Return Value:

NT status code.

--*/
{
	NTSTATUS            status = STATUS_SUCCESS;
	ULONG_PTR           bytesToCopy;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	DbgPrintGSL("HidGSLGetReportDescriptor Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfRequestRetrieveOutputMemory failed 0x%x\n", status);
		return status;
	}

	//
	// Use hardcoded Report descriptor
	//
	bytesToCopy = G_DefaultHidDescriptor.DescriptorList[0].wReportLength;

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		DbgPrint("G_DefaultHidDescriptor's reportLenght is zero, 0x%x\n", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0,
		(PVOID)G_DefaultReportDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfMemoryCopyFromBuffer failed 0x%x\n", status);
		return status;
	}
	
	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	DbgPrintGSL("HidGSLGetReportDescriptor Exit = 0x%x\n", status);
	return status;
}


NTSTATUS
HidGSLGetDeviceAttributes(
IN WDFREQUEST Request
)
/*++

Routine Description:

Fill in the given struct _HID_DEVICE_ATTRIBUTES

Arguments:

Request - Pointer to Request object.

Return Value:

NT status code.

--*/
{
	NTSTATUS                 status = STATUS_SUCCESS;
	PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;
	DbgPrintGSL("HidGSLGetDeviceAttributes Entry\n");
	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputBuffer(Request,
		sizeof (HID_DEVICE_ATTRIBUTES),
		&deviceAttributes,
		NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfRequestRetrieveOutputBuffer failed 0x%x\n", status);
		return status;
	}

	deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
	deviceAttributes->VendorID = VERDOR_ID_SILEAD;
	deviceAttributes->ProductID = PRODUCT_ID_SILEAD;
	deviceAttributes->VersionNumber = VERSION_NUMBER_SILEAD;

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, sizeof (HID_DEVICE_ATTRIBUTES));

	DbgPrintGSL("HidGSLGetDeviceAttributes Exit = 0x%x\n", status);
	return status;
}
/*
NTSTATUS
HidGSLGetInputReport(IN WDFREQUEST Request, WDFDEVICE device)
{
	NTSTATUS            status = STATUS_SUCCESS;
	ULONG           bytesToCopy;
	WDFMEMORY           memory;
	PDEVICE_EXTENSION             devContext = NULL;
	DbgPrint("HidGSLGetInputReport Entry\n");
	devContext = GetDeviceContext(device);
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfRequestRetrieveOutputMemory failed 0x%x\n", status);
		return status;
	}
	bytesToCopy = sizeof(devContext->PointDataDetail)/sizeof(BYTE);

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		DbgPrint("HidGSLGetInputReport's bytesToCopy is zero, 0x%x\n", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0,
		(PVOID)&devContext->PointDataDetail,//HidInputReport
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfMemoryCopyFromBuffer failed 0x%x\n", status);
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);
	DbgPrint("HidGSLGetInputReport Exit = 0x%x\n", status);
	return status;
}
*/

NTSTATUS
HidGSLGetString(
_In_ WDFDEVICE  FxDevice,
_In_ WDFREQUEST FxRequest
)
/*++

Routine Description:

Returns string requested by the HIDCLASS driver

Arguments:

FxDevice - Handle to WDF Device Object

FxRequest - Handle to request object

Return Value:

NTSTATUS indicating success or failure

--*/
{
	PIRP               irp;
	PIO_STACK_LOCATION irpSp;
	ULONG_PTR          lenId;
	NTSTATUS           status;
	PWSTR              strId;

	//change here
	const PWSTR ManufacturerID = L"Silead";
	const PWSTR ProductID = L"1680";
	const PWSTR SerialNumber = L"1";
/*
	const PWSTR ManufacturerID = L"Microsoft";
	const PWSTR ProductID = L"1680";
	const PWSTR SerialNumber = L"0";*/

	//FuncEntry(TRACE_FLAG_HID);

	UNREFERENCED_PARAMETER(FxDevice);

	status = STATUS_SUCCESS;

	irp = WdfRequestWdmGetIrp(FxRequest);
	irpSp = IoGetCurrentIrpStackLocation(irp);

	switch ((ULONG_PTR)irpSp->Parameters.DeviceIoControl.Type3InputBuffer &
		0xffff)
	{
	case HID_STRING_ID_IMANUFACTURER:
		strId = ManufacturerID;
		break;

	case HID_STRING_ID_IPRODUCT:
		strId = ProductID;
		break;

	case HID_STRING_ID_ISERIALNUMBER:
		strId = SerialNumber;
		break;

	default:
		strId = NULL;
		break;
	}

	lenId = strId ? (wcslen(strId)*sizeof(WCHAR)+sizeof(UNICODE_NULL)) : 0;
	if (NULL == strId)
	{
		status = STATUS_INVALID_PARAMETER;
	}
	else if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < lenId)
	{
		status = STATUS_BUFFER_TOO_SMALL;
	}
	else
	{
		RtlCopyMemory(irp->UserBuffer, strId, lenId);
		irp->IoStatus.Information = lenId;
	}

	if (!NT_SUCCESS(status))
	{
		DbgPrint("Error getting device string - %!STATUS!", status);
	}

	//FuncExit(TRACE_FLAG_HID);

	return status;
}

// change here - add feature report like below

NTSTATUS
GetFeatureReport(
_In_ WDFDEVICE  FxDevice,
_In_ WDFREQUEST FxRequest
)
/*++

Routine Description:

Emulates retrieval of a HID Feature report

Arguments:

FxDevice  - Framework device object
FxRequest - Framework request object

Return Value:

NTSTATUS indicating success or failure

--*/
{
	//PDEVICE_CONTEXT        pDeviceContext;
	PDEVICE_EXTENSION             devContext;
	PHID_XFER_PACKET       featurePacket;
	WDF_REQUEST_PARAMETERS params;
	NTSTATUS               status = STATUS_SUCCESS;

	DbgPrintGSL("GetFeatureReport Entry\n");
	
	devContext = GetDeviceContext(FxDevice);
	NT_ASSERT(devContext != NULL);
	

	//
	// Validate Request Parameters
	//

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(FxRequest, &params);
	DbgPrintGSL("GetFeatureReport entry====\n");

	if (params.Parameters.DeviceIoControl.OutputBufferLength <
		sizeof(HID_XFER_PACKET))
	{
		DbgPrint("GetFeatureReport，STATUS_BUFFER_TOO_SMALL====\n");
		status = STATUS_BUFFER_TOO_SMALL;
		goto exit;
	}

	featurePacket =
		(PHID_XFER_PACKET)WdfRequestWdmGetIrp(FxRequest)->UserBuffer;

	if (NULL == featurePacket)
	{
		DbgPrint("GetFeatureReport，STATUS_INVALID_DEVICE_REQUEST====\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		goto exit;
	}

	//
	// Process Request
	//
	switch (*(PUCHAR)featurePacket->reportBuffer)
	{
		case REPORTID_MAX_COUNT:
		{

			PHID_MAX_COUNT_REPORT maxCountReport =
				(PHID_MAX_COUNT_REPORT)featurePacket->reportBuffer;

			DbgPrintGSL("GetFeatureReport，REPORTID_MAX_COUNT====\n");
			if (featurePacket->reportBufferLen < sizeof(HID_MAX_COUNT_REPORT))
			{
				DbgPrint("GetFeatureReport，STATUS_BUFFER_TOO_SMALL====\n");
				status = STATUS_BUFFER_TOO_SMALL;
				goto exit;
			}

			maxCountReport->MaxCount = MAX_TOUCHES;
			WdfRequestSetInformation(FxRequest, sizeof (HID_MAX_COUNT_REPORT));
			break;
		}

		case REPORTID_FW_KEY:
		{

			PHID_FW_KEY_REPORT firmwareKey = (PHID_FW_KEY_REPORT)featurePacket->reportBuffer;

			DbgPrintGSL("GetFeatureReport，REPORTID_FW_KEY, featurePacket->reportBufferLen = %d ====\n", featurePacket->reportBufferLen);
			if (featurePacket->reportBufferLen < sizeof(HID_FW_KEY_REPORT))
			{
				DbgPrint("GetFeatureReport，STATUS_BUFFER_TOO_SMALL====\n");
				status = STATUS_BUFFER_TOO_SMALL;
				goto exit;
			}

			for (ULONG i = 0; i < FW_KEY_LENGTH; i++)
			{
				firmwareKey->FirmwareKey[i] = gsl_key[i];
			}
			WdfRequestSetInformation(FxRequest, sizeof (HID_FW_KEY_REPORT));
			break;
		}
		default:
		{
			DbgPrint("GetFeatureReport，STATUS_INVALID_PARAMETER====\n");
			status = STATUS_INVALID_PARAMETER;
			goto exit;
		}
	}

exit:
	DbgPrintGSL("GetFeatureReport Exit = 0x%x\n", status);
	return status;
}

PCHAR
DbgHidInternalIoctlString(
IN ULONG        IoControlCode
)
/*++

Routine Description:

Returns Ioctl string helpful for debugging

Arguments:

IoControlCode - IO Control code

Return Value:

Name String

--*/
{
	switch (IoControlCode)
	{
	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
	case IOCTL_HID_READ_REPORT:
		return "IOCTL_HID_READ_REPORT";
	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
	case IOCTL_HID_WRITE_REPORT:
		return "IOCTL_HID_WRITE_REPORT";
	case IOCTL_HID_SET_FEATURE:
		return "IOCTL_HID_SET_FEATURE";
	case IOCTL_HID_GET_FEATURE:
		return "IOCTL_HID_GET_FEATURE";
	case IOCTL_HID_GET_STRING:
		return "IOCTL_HID_GET_STRING";
	case IOCTL_HID_ACTIVATE_DEVICE:
		return "IOCTL_HID_ACTIVATE_DEVICE";
	case IOCTL_HID_DEACTIVATE_DEVICE:
		return "IOCTL_HID_DEACTIVATE_DEVICE";
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
		return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
	case IOCTL_HID_GET_INPUT_REPORT:
		return "IOCTL_HID_GET_INPUT_REPORT";
	default:
		return "Unknown IOCTL";
	}
}

