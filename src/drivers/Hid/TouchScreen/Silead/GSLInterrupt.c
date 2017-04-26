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
#include <wdm.h>
#include <wdf.h>
#include <hidport.h>
#include "trace.h"
#ifdef DEBUG_C_TMH
#include "GSLInterrupt.tmh"
#endif

#if 0
VOID DebugReportData(_In_ WDFDEVICE device, _Inout_ PHIDFX2_INPUT_REPORT pInputReport)
{
	PDEVICE_EXTENSION	devContext = NULL;
	devContext = GetDeviceContext(device);
	DbgPrint("#######Report data, ContactCount: %d \n", pInputReport->ContactCount);
	DbgPrint("Tip0: %d, ContactId0: %d, XLSB0: %d, XMSB0: %d, YLSB0: %d, YMSB0: %d \n",
		pInputReport->Tip0, pInputReport->ContactId0,
		pInputReport->XLSB0, pInputReport->XMSB0,
		pInputReport->YLSB0, pInputReport->YMSB0
		);
	DbgPrint("Tip1: %d, ContactId1: %d, XLSB1: %d, XMSB1: %d, YLSB1: %d, YMSB1: %d \n",
		pInputReport->Tip1, pInputReport->ContactId1,
		pInputReport->XLSB1, pInputReport->XMSB1,
		pInputReport->YLSB1, pInputReport->YMSB1
		);
	DbgPrint("Tip2: %d, ContactId2: %d, XLSB2: %d, XMSB2: %d, YLSB2: %d, YMSB2: %d \n",
		pInputReport->Tip2, pInputReport->ContactId2,
		pInputReport->XLSB2, pInputReport->XMSB2,
		pInputReport->YLSB2, pInputReport->YMSB2
		);
	DbgPrint("Tip3: %d, ContactId3: %d, XLSB3: %d, XMSB3: %d, YLSB3: %d, YMSB3: %d \n",
		pInputReport->Tip3, pInputReport->ContactId3,
		pInputReport->XLSB3, pInputReport->XMSB3,
		pInputReport->YLSB3, pInputReport->YMSB3
		);
	DbgPrint("Tip4: %d, ContactId4: %d, XLSB4: %d, XMSB4: %d, YLSB4: %d, YMSB4: %d \n\n",
		pInputReport->Tip4, pInputReport->ContactId4,
		pInputReport->XLSB4, pInputReport->XMSB4,
		pInputReport->YLSB4, pInputReport->YMSB4
		);
	/*DbgPrint("ScanTimeLSB: %d, ScanTimeMSB: %d \n",
		pInputReport->ScanTimeLSB, pInputReport->ScanTimeMSB);*/
}
#endif

#ifdef SUPPORT_WINKEY
BYTE GSLProcessData(_In_ WDFDEVICE device, _Inout_ PHIDFX2_INPUT_REPORT pInputReport, _Inout_ PHIDFX2_INPUT_REPORT_EX pInputReportEx)
#else
BYTE GSLProcessData(_In_ WDFDEVICE device, _Inout_ PHIDFX2_INPUT_REPORT pInputReport)
#endif
{
	UCHAR	i, id, xlsb, xmsb, ylsb, ymsb;
	UCHAR	touch_data[4 + MAX_CONTACTS*4];
	PDEVICE_EXTENSION	devContext;
	NTSTATUS Status = STATUS_SUCCESS;
	UCHAR touches;
	UINT32 tiaoping;
	UCHAR buf[4] = {0};
	struct gsl_touch_info cinfo = {0};
	BYTE process_status = 1;
	BOOLEAN bUP = FALSE;
	DbgPrintGSL("%s Entry\n", __FUNCTION__);

	devContext = GetDeviceContext(device);

#ifdef SUPPORT_WINKEY	
	pInputReportEx->ReportTouchId = REPORTID_WINKEY;
	pInputReportEx->dwButtonStates = 0;
	pInputReportEx->Reserved = 0;
	for(i = 0; i < 6; i ++)
		pInputReportEx->KeyCode[i] = 0;
#endif
	pInputReport->ReportTouchId = REPORTID_INPUT;
	pInputReport->Tip0 = 0;
	pInputReport->Tip1 = 0;
	pInputReport->Tip2 = 0;
	pInputReport->Tip3 = 0;
	pInputReport->Tip4 = 0;
#ifndef	SET_MULTITOUCH_5POINT
	pInputReport->Tip5 = 0;
	pInputReport->Tip6 = 0;
	pInputReport->Tip7 = 0;
	pInputReport->Tip8 = 0;
	pInputReport->Tip9 = 0;
#endif
	pInputReport->ContactId0 = 0;
	pInputReport->ContactId1 = 0;
	pInputReport->ContactId2 = 0;
	pInputReport->ContactId3 = 0;
	pInputReport->ContactId4 = 0;
#ifndef	SET_MULTITOUCH_5POINT
	pInputReport->ContactId5 = 0;
	pInputReport->ContactId6 = 0;
	pInputReport->ContactId7 = 0;
	pInputReport->ContactId8 = 0;
	pInputReport->ContactId9 = 0;
#endif

	/*
	1.clear new to 0,Point_Data[0][0-9]
	2.assign new, Point_Data[0][id[i]-1]
	3.judge up or down
	4.new becomes old.
	remark: Point_Data[0][0-9] store new data, Point_Data[1][0-9] store old data
	*/

	{	//step1: clear new to 0, Point_Data[0][0 - 9]
		UINT8 i1 = 0;
		UINT8 j = 0;
		for (; i1 < PS_DEEP-1; i1++)
		{
			for (; j < MAX_CONTACTS; j++)
			{
				devContext->Point_Data[i1][j].id = 0;
				devContext->Point_Data[i1][j].x = 0;
				devContext->Point_Data[i1][j].y = 0;
			}
		}
	}
	Status = GSLDevReadBuffer(device, 0x80, touch_data, sizeof(touch_data));
	if (!NT_SUCCESS(Status)) {
		DbgPrint("read touch data error! Status = 0x%x\n", Status);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "GSLProcessData  read touch data error! Status = 0x%x\n", Status);
		return 0;
	}
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "GSLProcessData  read touch data sucessful! Status = 0x%x\n", Status);
#ifdef GSL_DEBUG
	{
		UINT16 i2 = 0;
		for (; i2 < 8; i2++)
		{
			//DbgPrint("----touch_data[%d] = 0x%2x \n", i2, touch_data[i2]);
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "----touch_data[%d] = 0x%2x \n", i2, touch_data[i2]);
		}
	}
#endif
	touches = touch_data[0];
	
	if(devContext->NoIDVersion)
	{
		for(i = 0; i < (touches < MAX_CONTACTS ? touches : MAX_CONTACTS); i ++)
		{
			cinfo.id[i] = (touch_data[4 + i*4 + 3]>>4)&0xf;
			cinfo.x[i] = (touch_data[4 + i*4 + 3]&0xf)*256 + touch_data[4 + i*4 + 2];
			cinfo.y[i] = touch_data[4 + i*4 + 1]*256+ touch_data[4 + i*4 + 0];

			//DbgPrintGSL("==========before x[%d] = %d, y[%d] = %d, id[%d] = %d ==========\n", i, cinfo.x[i], i, cinfo.y[i], i, cinfo.id[i]);
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "==========before x[%d] = %d, y[%d] = %d, id[%d] = %d ==========\n", i, cinfo.x[i], i, cinfo.y[i], i, cinfo.id[i]);
		}
		cinfo.finger_num = (touch_data[3]<<24)|(touch_data[2]<<16)|(touch_data[1]<<8)|(touch_data[0]);
		gsl_alg_id_main(&cinfo);
		tiaoping = gsl_mask_tiaoping();
		if(tiaoping > 0 && tiaoping < 0xffffffff)
		{
			buf[0]=0xa;buf[1]=0;buf[2]=0;buf[3]=0;
			GSLDevWriteBuffer(device, 0xf0, buf, 4);
			buf[0]=(UCHAR)(tiaoping & 0xff);
			buf[1]=(UCHAR)((tiaoping>>8) & 0xff);
			buf[2]=(UCHAR)((tiaoping>>16) & 0xff);
			buf[3]=(UCHAR)((tiaoping>>24) & 0xff);
			DbgPrint("tiaoping=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n", tiaoping,buf[0],buf[1],buf[2],buf[3]);
			GSLDevWriteBuffer(device, 0x8, buf, 4);
		}
		DbgPrintGSL("----cinfo.finger_num = %d\n", cinfo.finger_num);
		touches = (UCHAR)cinfo.finger_num;
	}	
	if (touches > MAX_TOUCHES)
		touches = MAX_TOUCHES;

	DbgPrintGSL("----ContactCount = %d\n", touches);
	if(touches == 0)
		DbgPrintGSL("=======raw data, finger_number = 0 =======\n");

	pInputReport->ScanTimeLSB = (devContext->ScanTime)&0xff;
	pInputReport->ScanTimeMSB = ((devContext->ScanTime)>>8)&0xff;
	DbgPrintGSL("----devContext->ScanTime = %d, ScanTimeMSB = %d, ScanTimeLSB = %d\n", devContext->ScanTime, pInputReport->ScanTimeMSB, pInputReport->ScanTimeLSB);

	for(i = 0; i < touches; i ++)
	{
		if(devContext->NoIDVersion)
		{		
			id = (UCHAR)cinfo.id[i];
			xmsb = (UCHAR)(cinfo.x[i]>>8);
			xlsb = (UCHAR)(cinfo.x[i]&0xff);
			ymsb = (UCHAR)(cinfo.y[i]>>8);
			ylsb = (UCHAR)(cinfo.y[i] & 0xff);
			if (id > 0)
			{
				devContext->Point_Data[0][(UINT8)(id - 1)].id = (UINT8)id;
				cinfo.x[i] = 800 - cinfo.x[i];
				devContext->Point_Data[0][(UINT8)(id - 1)].x = cinfo.x[i];
				cinfo.y[i] =1280 - cinfo.y[i];
				devContext->Point_Data[0][(UINT8)(id - 1)].y = cinfo.y[i];
			}
			//DbgPrintGSL("==========after x[%d] = %d, y[%d] = %d, id[%d] = %d ==========\n", i, cinfo.x[i], i, cinfo.y[i], i, id);
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "==========after x[%d] = %d, y[%d] = %d, id[%d] = %d ==========\n", i, cinfo.x[i], i, cinfo.y[i], i, id);

		}
		else
		{		
			id = (touch_data[4 + i*4 + 3]>>4)&0xf;
			xmsb = touch_data[4 + i*4 + 3]&0xf;
			xlsb = touch_data[4 + i*4 + 2];
			ymsb = touch_data[4 + i*4 + 1];
			ylsb = touch_data[4 + i*4 + 0];
		}
 		DbgPrintGSL("=======raw data, id = %d, xmsb = %d, xlsb = %d, ymsb = %d, ylsb = %d=======\n", id, xmsb, xlsb, ymsb, ylsb);

#ifdef SUPPORT_WINKEY		
		if((cinfo.x[i] > (WINKEY_CENTER_X - WINKEY_WIDTH_X/2))
		&& (cinfo.x[i] < (WINKEY_CENTER_X + WINKEY_WIDTH_X/2))
		&& (cinfo.y[i] > (WINKEY_CENTER_Y - WINKEY_WIDTH_Y/2))
		&& (cinfo.y[i] < (WINKEY_CENTER_Y + WINKEY_WIDTH_Y/2))
		)
		{
			DbgPrintGSL("==========WINKEY=============\n");
			pInputReportEx->dwButtonStates = 0x08;
			process_status = 2;
			break;
		}
#endif
	}
	//step3: judge up or down
	for (i = 0; i < MAX_TOUCHES; i++)
	{
		if (devContext->Point_Data[0][i].x != 0 || devContext->Point_Data[0][i].y != 0)	//new data !=0, it is down.
		{			
			if (pInputReport->XLSB0 == 0 && pInputReport->XMSB0 == 0 && pInputReport->YLSB0 == 0 && pInputReport->YMSB0 == 0 && pInputReport->ContactId0 == 0)
			{
				pInputReport->Tip0 = 1;
				pInputReport->ContactId0 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB0 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB0 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB0 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB0 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB1 == 0 && pInputReport->XMSB1 == 0 && pInputReport->YLSB1 == 0 && pInputReport->YMSB1 == 0 && pInputReport->ContactId1 == 0)
			{
				pInputReport->Tip1 = 1;
				pInputReport->ContactId1 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB1 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB1 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB1 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB1 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB2 == 0 && pInputReport->XMSB2 == 0 && pInputReport->YLSB2 == 0 && pInputReport->YMSB2 == 0 && pInputReport->ContactId2 == 0)
			{
				pInputReport->Tip2 = 1;
				pInputReport->ContactId2 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB2 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB2 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB2 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB2 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB3 == 0 && pInputReport->XMSB3 == 0 && pInputReport->YLSB3 == 0 && pInputReport->YMSB3 == 0 && pInputReport->ContactId3 == 0)
			{
				pInputReport->Tip3 = 1;
				pInputReport->ContactId3 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB3 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB3 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB3 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB3 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB4 == 0 && pInputReport->XMSB4 == 0 && pInputReport->YLSB4 == 0 && pInputReport->YMSB4 == 0 && pInputReport->ContactId4 == 0)
			{
				pInputReport->Tip4 = 1;
				pInputReport->ContactId4 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB4 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB4 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB4 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB4 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
#ifndef	SET_MULTITOUCH_5POINT
			if (pInputReport->XLSB5 == 0 && pInputReport->XMSB5 == 0 && pInputReport->YLSB5 == 0 && pInputReport->YMSB5 == 0 && pInputReport->ContactId5 == 0)
			{
				pInputReport->Tip5 = 1;
				pInputReport->ContactId5 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB5 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB5 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB5 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB5 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB6 == 0 && pInputReport->XMSB6 == 0 && pInputReport->YLSB6 == 0 && pInputReport->YMSB6 == 0 && pInputReport->ContactId6 == 0)
			{
				pInputReport->Tip6 = 1;
				pInputReport->ContactId6 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB6 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB6 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB6 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB6 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB7 == 0 && pInputReport->XMSB7 == 0 && pInputReport->YLSB7 == 0 && pInputReport->YMSB7 == 0 && pInputReport->ContactId7 == 0)
			{
				pInputReport->Tip7 = 1;
				pInputReport->ContactId7 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB7 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB7 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB7 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB7 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB8 == 0 && pInputReport->XMSB8 == 0 && pInputReport->YLSB8 == 0 && pInputReport->YMSB8 == 0 && pInputReport->ContactId8 == 0)
			{
				pInputReport->Tip8 = 1;
				pInputReport->ContactId8 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB8 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB8 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB8 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB8 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB9 == 0 && pInputReport->XMSB9 == 0 && pInputReport->YLSB9 == 0 && pInputReport->YMSB9 == 0 && pInputReport->ContactId9 == 0)
			{
				pInputReport->Tip9 = 1;
				pInputReport->ContactId9 = devContext->Point_Data[0][i].id;
				pInputReport->XLSB9 = (UCHAR)(devContext->Point_Data[0][i].x & 0xff);
				pInputReport->XMSB9 = (UCHAR)(devContext->Point_Data[0][i].x >> 8);
				pInputReport->YLSB9 = (UCHAR)(devContext->Point_Data[0][i].y & 0xff);
				pInputReport->YMSB9 = (UCHAR)(devContext->Point_Data[0][i].y >> 8);
				continue;
			}
#endif
		}
		else if (devContext->Point_Data[1][i].x != 0 || devContext->Point_Data[1][i].y != 0)
		{
			bUP = TRUE;
			//new data = 0, old != 0, it is up.
			if (pInputReport->XLSB0 == 0 && pInputReport->XMSB0 == 0 && pInputReport->YLSB0 == 0 && pInputReport->YMSB0 == 0 && pInputReport->ContactId0 == 0)
			{
				pInputReport->Tip0 = 0;
				pInputReport->ContactId0 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB0 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB0 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB0 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB0 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB1 == 0 && pInputReport->XMSB1 == 0 && pInputReport->YLSB1 == 0 && pInputReport->YMSB1 == 0 && pInputReport->ContactId1 == 0)
			{
				pInputReport->Tip1 = 0;
				pInputReport->ContactId1 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB1 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB1 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB1 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB1 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB2 == 0 && pInputReport->XMSB2 == 0 && pInputReport->YLSB2 == 0 && pInputReport->YMSB2 == 0 && pInputReport->ContactId2 == 0)
			{
				pInputReport->Tip2 = 0;
				pInputReport->ContactId2 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB2 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB2 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB2 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB2 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB3 == 0 && pInputReport->XMSB3 == 0 && pInputReport->YLSB3 == 0 && pInputReport->YMSB3 == 0 && pInputReport->ContactId3 == 0)
			{
				pInputReport->Tip3 = 0;
				pInputReport->ContactId3 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB3 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB3 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB3 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB3 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB4 == 0 && pInputReport->XMSB4 == 0 && pInputReport->YLSB4 == 0 && pInputReport->YMSB4 == 0 && pInputReport->ContactId4 == 0)
			{
				pInputReport->Tip4 = 0;
				pInputReport->ContactId4 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB4 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB4 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB4 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB4 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
#ifndef	SET_MULTITOUCH_5POINT			
			if (pInputReport->XLSB5 == 0 && pInputReport->XMSB5 == 0 && pInputReport->YLSB5 == 0 && pInputReport->YMSB5 == 0 && pInputReport->ContactId5 == 0)
			{
				pInputReport->Tip5 = 0;
				pInputReport->ContactId5 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB5 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB5 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB5 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB5 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB6 == 0 && pInputReport->XMSB6 == 0 && pInputReport->YLSB6 == 0 && pInputReport->YMSB6 == 0 && pInputReport->ContactId6 == 0)
			{
				pInputReport->Tip6 = 0;
				pInputReport->ContactId6 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB6 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB6 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB6 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB6 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB7 == 0 && pInputReport->XMSB7 == 0 && pInputReport->YLSB7 == 0 && pInputReport->YMSB7 == 0 && pInputReport->ContactId7 == 0)
			{
				pInputReport->Tip7 = 0;
				pInputReport->ContactId7 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB7 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB7 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB7 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB7 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB8 == 0 && pInputReport->XMSB8 == 0 && pInputReport->YLSB8 == 0 && pInputReport->YMSB8 == 0 && pInputReport->ContactId8 == 0)
			{
				pInputReport->Tip8 = 0;
				pInputReport->ContactId8 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB8 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB8 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB8 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB8 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
			if (pInputReport->XLSB9 == 0 && pInputReport->XMSB9 == 0 && pInputReport->YLSB9 == 0 && pInputReport->YMSB9 == 0 && pInputReport->ContactId9 == 0)
			{
				pInputReport->Tip9 = 0;
				pInputReport->ContactId9 = devContext->Point_Data[1][i].id;
				pInputReport->XLSB9 = (UCHAR)(devContext->Point_Data[1][i].x & 0xff);
				pInputReport->XMSB9 = (UCHAR)(devContext->Point_Data[1][i].x >> 8);
				pInputReport->YLSB9 = (UCHAR)(devContext->Point_Data[1][i].y & 0xff);
				pInputReport->YMSB9 = (UCHAR)(devContext->Point_Data[1][i].y >> 8);
				continue;
			}
#endif
		}
	}
	devContext->ContactCount[0] = touches;
	if (bUP == TRUE)
		pInputReport->ContactCount = devContext->ContactCount[1];
	else
		pInputReport->ContactCount = devContext->ContactCount[0];

	//step4: new becomes old.
	for (i = 0; i < MAX_TOUCHES; i++)
	{
		devContext->Point_Data[1][i].x = devContext->Point_Data[0][i].x;
		devContext->Point_Data[1][i].y = devContext->Point_Data[0][i].y;
		devContext->Point_Data[1][i].id = devContext->Point_Data[0][i].id;
	}
	devContext->ContactCount[1] = devContext->ContactCount[0];

#ifdef SCANTIME_CONSTANT
	if(devContext->ScanTime < 65535 && 0 != pInputReport->ContactCount)
		devContext->ScanTime += SCANTIME_STEP;
	if (devContext->ScanTime > 65535 || 0 == pInputReport->ContactCount)
		devContext->ScanTime = 0;
#endif

#ifdef SCANTIME_SYSTEM
	//ScanTime for WHQL test
	{
		if (pInputReport->ContactCount == 0)
		{
			devContext->bStartTime = FALSE;
			devContext->ScanTime = 0;
		}
		else
		{
			LARGE_INTEGER StartTime;
			LARGE_INTEGER TickTime;
			LARGE_INTEGER LocalStartTime;
			LARGE_INTEGER LocalTickTime;
			ULONG deltaMs;
			if (devContext->bStartTime == FALSE)
			{
				KeQuerySystemTime(&StartTime);
				ExSystemTimeToLocalTime(&StartTime, &LocalStartTime);
				devContext->StartTime.QuadPart = LocalStartTime.QuadPart / 10000;
				devContext->bStartTime = TRUE;
			}
			else
			{
				KeQuerySystemTime(&TickTime);
				ExSystemTimeToLocalTime(&TickTime, &LocalTickTime);
				deltaMs = (ULONG)(LocalTickTime.QuadPart / 10000 - devContext->StartTime.QuadPart);
				devContext->ScanTime = deltaMs * 10;
				DbgPrintGSL("---- %d ms\n", deltaMs);
			}
		}
		if (devContext->ScanTime > 65535)
		{
			devContext->ScanTime = 0;
			devContext->bStartTime = FALSE;
			DbgPrintGSL("ScanTime is timeout\n");
		}
	}
#endif

#ifdef SUPPORT_WINKEY
	if(devContext->ButtonDown  == TRUE)
	{
		if(process_status != 2)
		{
			devContext->ButtonDown  = FALSE;
			process_status = 2;
		}
	}
	else
	{
		if(process_status == 2)	
			devContext->ButtonDown  = TRUE;
	}
#endif
#if 0
	{
		DebugReportData(device, pInputReport);
	}
#endif	
	return process_status;
}

/*
when powering up a device,KMDF invokes a driver's callback in the following order:
1.D0Entry
2.InterruptEnable(DIRQL)
3.D0EntryPostInteruptEnabled(PASSIVE LEVEL)
when powering down a device,KMDF invokes a driver's callback in the following order:
1.D0Exit
2.InterruptDisable(DIRQL)
3.D0EntryPostInteruptDisabled(PASSIVE LEVEL)
*/
_Use_decl_annotations_ BOOLEAN 
GSLDevInterruptIsr(_In_ WDFINTERRUPT Interrupt, _In_ ULONG MessageID)
{
	//this callback runs at DIRQL greater than APC level.
	//queue a DPC to perform any work related to the interrupt.
	WDFDEVICE			device;
	PDEVICE_EXTENSION	devContext = NULL;
	BOOLEAN				InterruptRecognized = FALSE;
	NTSTATUS			status;
	WDFREQUEST			request = NULL;
	WDFMEMORY			reqMemory;
	HIDFX2_INPUT_REPORT	InputReport = { 0 };
#ifdef SUPPORT_WINKEY
	HIDFX2_INPUT_REPORT_EX	InputReportEx = { 0 };
#endif
	BYTE process_status;
	
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID,0,"%s SSSSSSSSSSSSSSSSSSSS GSLDevInterruptIsr \n", __FUNCTION__);
	UNREFERENCED_PARAMETER(MessageID);
	
	device = WdfInterruptGetDevice(Interrupt);
	devContext = GetDeviceContext(device);
	InterruptRecognized = TRUE;

#ifdef GSL_TIMER
	if(devContext->i2c_lock != 0)
	{
		goto i2c_lock;
	}
	else
	{
		devContext->i2c_lock = 1;
	}
#endif
	status = WdfIoQueueRetrieveNextRequest(
		devContext->ReportQueue,
		&request);
	if (!NT_SUCCESS(status) || (request == NULL)) {
		DbgPrintGSL("No requests to process, WdfIoQueueRetrieveNextRequest failed status: 0x%x\n", status);
		goto exit;// No requests to process. 
	}
#ifdef SUPPORT_WINKEY
	process_status = GSLProcessData(device, &InputReport, &InputReportEx);
#else
	process_status = GSLProcessData(device, &InputReport);
#endif
	DbgPrintGSL("GSLProcessData, process_status = %d\n", process_status);
	if (1 == process_status)
	{
		//
		// Retrieve the request buffer
		//
		status = WdfRequestRetrieveOutputMemory(request, &reqMemory);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("WdfRequestRetrieveOutputMemory failed status: 0x%x\n", status);
			goto requestComplete;
		}

		//
		// Copy the input report into the request buffer
		status = WdfMemoryCopyFromBuffer(
			reqMemory,
			0,
			(PVOID)&InputReport,
			sizeof(HIDFX2_INPUT_REPORT));
		if (!NT_SUCCESS(status))
		{
			DbgPrint("WdfMemoryCopyFromBuffer failed status: 0x%x\n", status);
			goto requestComplete;
		}

		//
		// Report how many bytes were copied
		//
		WdfRequestSetInformation(request, sizeof(HIDFX2_INPUT_REPORT));
	}
#ifdef SUPPORT_WINKEY
	else if(2 == process_status)
	{
		//
		// Retrieve the request buffer
		//
		status = WdfRequestRetrieveOutputMemory(request, &reqMemory);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("WdfRequestRetrieveOutputMemory failed status: 0x%x\n", status);
			goto requestComplete;
		}

		//
		// Copy the input report into the request buffer
		status = WdfMemoryCopyFromBuffer(
			reqMemory,
			0,
			(PVOID)&InputReportEx,
			sizeof(HIDFX2_INPUT_REPORT_EX));
		if (!NT_SUCCESS(status))
		{
			DbgPrint("WdfMemoryCopyFromBuffer failed status: 0x%x\n", status);
			goto requestComplete;
		}

		//
		// Report how many bytes were copied
		//
		WdfRequestSetInformation(request, sizeof(HIDFX2_INPUT_REPORT_EX));
	}
#endif
	else
	{
		goto exit;
	}

requestComplete:
	status = WdfRequestForwardToIoQueue(request, devContext->CompletionQueue);
	if (!NT_SUCCESS(status)) {
		//
		// CompletionQueue is a manual-dispatch, non-power-managed queue. This should*never* fail.
		//
		DbgPrint("WdfRequestForwardToIoQueue failed status: 0x%x\n", status);
		goto exit;
	}
	WdfInterruptQueueDpcForIsr(Interrupt);
exit:
#ifdef GSL_TIMER
	devContext->i2c_lock = 0;
i2c_lock:
#endif
	return InterruptRecognized;
}

VOID
GSLDevInterruptDpc(_In_ WDFINTERRUPT Interrupt, _In_ WDFOBJECT AssociatedObject)
{
	//this callback runs at DISPATCH_LEVEL
	//WdfRequestComplete 
	WDFDEVICE			device;
	PDEVICE_EXTENSION	devContext;
	WDFREQUEST			request;
	NTSTATUS			status;
	BOOLEAN				run, rerun;
	
	UNREFERENCED_PARAMETER(AssociatedObject);
	DbgPrintGSL("%s Entry\n", __FUNCTION__);

	device = WdfInterruptGetDevice(Interrupt);
	devContext = GetDeviceContext(device);

	WdfSpinLockAcquire(devContext->DpcLock);
	if (devContext->DpcInProgress) {
		devContext->DpcRerun = TRUE;
		run = FALSE;
	}
	else {
		devContext->DpcInProgress = TRUE;
		devContext->DpcRerun = FALSE;
		run = TRUE;
	}
	WdfSpinLockRelease(devContext->DpcLock);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 1, "%s been called\n", __FUNCTION__);
	if (run == FALSE) {
		DbgPrint("%s in the completion queue, run = FALSE\n", __FUNCTION__);
		return;
	}
	DbgPrintGSL("%s in the completion queue, DpcInProgress = %d, DpcRerun = %d \n", __FUNCTION__, devContext->DpcInProgress, devContext->DpcRerun);

	do
	{
		for (;;)
		{
			//
			//complete all reports in the completion queue
			//
			request = NULL;
			status = WdfIoQueueRetrieveNextRequest(
				devContext->CompletionQueue,
				&request);
			if (!NT_SUCCESS(status) || (request == NULL)) {
				DbgPrintGSL("No requests to process£¬WdfIoQueueRetrieveNextRequest failed status: 0x%x\n", status);
				break;// No requests to process. 
			}
			DbgPrintGSL("%s in the completion queue£¬success\n", __FUNCTION__);
			WdfRequestComplete(request, STATUS_SUCCESS);
		}

		WdfSpinLockAcquire(devContext->DpcLock);
		if (devContext->DpcRerun) {
			rerun = TRUE;
			devContext->DpcRerun = FALSE;
			DbgPrintGSL("%s in the completion queue DpcRerun = 1 =====\n", __FUNCTION__);
		}
		else {
			devContext->DpcInProgress = FALSE;
			rerun = FALSE;
			DbgPrintGSL("%s in the completion queue DpcRerun = 0 =====\n", __FUNCTION__);
		}
		WdfSpinLockRelease(devContext->DpcLock);
	} while (rerun);
}

