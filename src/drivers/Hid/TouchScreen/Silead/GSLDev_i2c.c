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

#include <ntddk.h>
#include <wdf.h>
#include <gpioclx.h>
#define RESHUB_USE_HELPER_ROUTINES
#include "reshub.h"  // Resource and descriptor definitions
#ifdef DEBUG_C_TMH
#include "GSLDev_i2c.tmh"
#endif
#include "Device.h"
#include "trace.h"
#include "Hiddesc.h"
#define XOR 0x88
//#include "GSL_TS_CFG.h"

#ifdef PORTRIAT_RESOLUTION
	#if defined(GSL1680)
		#include "default_tp_config\portrait\GSL1680E.h"
#elif defined(GSL2681)
		#include "default_tp_config\portrait\GSL2681B.h"
#elif defined(GSL3670)
		#include "GSL_TS_CFG.h"
		//#include "default_tp_config\portrait\GSL3670.h"
#elif defined(GSL3673)
		#include "default_tp_config\portrait\GSL3673.h"
#elif defined(GSL3675)
		#include "GSL_TS_CFG.h"
		//#include "default_tp_config\portrait\GSL3675.h"
#elif defined(GSL3676)
		#include "GSL_TS_CFG.h"
		//#include "default_tp_config\portrait\GSL3675.h"
#elif defined(GSL3680)
		#include "default_tp_config\portrait\GSL3680B.h"
#elif defined(GSL3692)
		#include "default_tp_config\portrait\GSL3692.h"
#elif defined(GSL5680)
		#include "default_tp_config\portrait\GSL5680.h"
	#else
		#error no defined ic
	#endif
#elif defined(LANSCAPE_RESOLUTION)
	#if defined(GSL1680)
		#include "default_tp_config\landscape\GSL1680E.h"
	#elif defined(GSL2681)
		#include "default_tp_config\landscape\GSL2681B.h"
	#elif defined(GSL3670)
		#include "default_tp_config\landscape\GSL3670.h"
	#elif defined(GSL3673)
		#include "default_tp_config\landscape\GSL3673.h"
	#elif defined(GSL3675)
		#include "default_tp_config\landscape\GSL3675.h"
	#elif defined(GSL3676)
		#include "default_tp_config\landscape\GSL3676.h"
	#elif defined(GSL3680)
		#include "default_tp_config\landscape\GSL3680B.h"
	#elif defined(GSL3692)
		#include "default_tp_config\landscape\GSL3692.h"
	#elif defined(GSL5680)
		#include "default_tp_config\landscape\GSL5680.h"
	#else
		#error no defined ic
	#endif
#else
	#error not defined portrait or landscape
#endif

/*
#pragma warning( push )
#pragma warning( disable : 4701 )
// line of code generating the warning goes here
#pragma warning( pop ) 
*/

NTSTATUS
GSLDevWriteBuffer(
IN WDFDEVICE    Device,
_In_ BYTE RegisterAddress,
_In_reads_(DataLength) PUCHAR PData,
_In_ ULONG DataLength
)
{
	PUCHAR Buffer;
	ULONG BufferLength;
	ULONG_PTR BytesWritten;
	WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
	WDFMEMORY MemoryWrite = NULL;
	NTSTATUS Status;
	PDEVICE_EXTENSION             devContext = NULL;
	UINT16 i;
	
	devContext = GetDeviceContext(Device);
	BufferLength = DataLength + sizeof(BYTE);
	Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
		NonPagedPoolNx,
		GSL_GPIO_POOL_TAG,
		BufferLength,
		&MemoryWrite,
		(PVOID*)&Buffer);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("%s: WdfMemoryCreate failed allocating memory buffer for write!"
			"Status:%x\n",
			__FUNCTION__,
			Status);
		goto GSLWriteEnd;
	}

	RtlCopyMemory(Buffer, &RegisterAddress, sizeof(BYTE));
	if(PData != NULL)
		RtlCopyMemory((Buffer + sizeof(BYTE)), PData, DataLength);
	
	WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&MemoryDescriptor, MemoryWrite, NULL); //chnage here
	//if write first failed, write again. 
	for (i = 0; i < 2; i++)
	{
		Status = WdfIoTargetSendWriteSynchronously(
			devContext->I2CIOTarget,
			NULL,
			&MemoryDescriptor,
			NULL,
			NULL,
			&BytesWritten);

		if (!NT_SUCCESS(Status)) {
			DbgPrint("%s: WdfIoTargetSendWriteSynchronously failed! Status = 0x%x\n",
				__FUNCTION__,
				Status);
			//added by guoying to debug the driver
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s: WdfIoTargetSendWriteSynchronously failed! Status = 0x%x\n",
			//	__FUNCTION__,
			//	Status);
			continue;
		}
		else
		{
			break;
		}
	}

GSLWriteEnd:
	if (MemoryWrite != NULL) {
		WdfObjectDelete(MemoryWrite);
	}
	return Status;
}

NTSTATUS
GSLDevReadBuffer(
IN WDFDEVICE    Device,
_In_ BYTE RegisterAddress,
_Out_writes_(DataLength) PUCHAR PData,
_In_ USHORT DataLength
)
{
	PUCHAR Buffer;
	ULONG_PTR BytesRead = 0;
	WDF_MEMORY_DESCRIPTOR MemoryDescriptor;
	WDFMEMORY MemoryRead = NULL;
	NTSTATUS Status;
	UINT16 i;
	PDEVICE_EXTENSION             devContext;
	devContext = GetDeviceContext(Device);

	Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
		NonPagedPoolNx,
		GSL_GPIO_POOL_TAG,
		DataLength,
		&MemoryRead, 
		NULL);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("%s: WdfMemoryCreate failed allocating memory buffer for write!"
			"Status:%x\n",
			__FUNCTION__,
			Status);
		goto GSLReadEnd;
	}

	WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&MemoryDescriptor, MemoryRead, NULL);
	Buffer = WdfMemoryGetBuffer(MemoryRead, (size_t*)&DataLength);

	//if read first failed,read again. 
	for (i = 0; i < 2; i++)
	{
		Status = GSLDevWriteBuffer(Device, RegisterAddress, NULL, 0);
		if (!NT_SUCCESS(Status)) {
			DbgPrint("%s:  GSLDevWriteBuffer reg = 0x%02x failed! i = %d, Status = %#x\n",
				__FUNCTION__,
				RegisterAddress,i,
				Status);
			continue;
		}
		Status = WdfIoTargetSendReadSynchronously(
			devContext->I2CIOTarget,
			NULL,
			&MemoryDescriptor,
			NULL,
			NULL,
			&BytesRead);

		if (!NT_SUCCESS(Status)) {
			DbgPrint("%s: WdfIoTargetSendReadSynchronously read failed! Status = %x\n",
				__FUNCTION__,
				Status);
			//added by guoying to debug the driver
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s: WdfIoTargetSendReadSynchronously read failed! Status = %x\n",
			//	__FUNCTION__,
			//	Status);
		}
		else if(BytesRead == DataLength)
		{
			break;
		}
	}
	if (!NT_SUCCESS(Status)){
		DbgPrint("%s read failed! Status = %x\n",
			__FUNCTION__,
			Status);
		goto GSLReadEnd;
	}
	
	RtlCopyMemory(PData, Buffer, DataLength);

GSLReadEnd:
	if (MemoryRead != NULL) {
		WdfObjectDelete(MemoryRead);
	}
	return Status;

}

NTSTATUS
GSL_Test_I2C(IN WDFDEVICE    Device)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UCHAR buf;
	
	GSLDevReadBuffer(Device, 0xf0, &buf, 1);
	//DbgPrint("!!!!!!!!read 0xf0 is 0x%02x\n", buf);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "!!!!!!!!read 0xf0 is 0x%02x\n", buf);
	wd_delay(2);
	buf = 0x12;
	GSLDevWriteBuffer(Device, 0xf0, &buf, 1);
	//DbgPrint("!!!!!!!!write 0xf0 is 0x%02x\n", buf);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "!!!!!!!!write 0xf0 is 0x%02x\n", buf);
	wd_delay(2);
	Status = GSLDevReadBuffer(Device, 0xf0, &buf, 1);
	//DbgPrint("!!!!!!!!read 0xf0 is 0x%02x\n", buf);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "!!!!!!!!read 0xf0 is 0x%02x\n", buf);

	return Status;
}
NTSTATUS
GSL_Clr_Reg(IN WDFDEVICE    Device)
{
	UCHAR buf[4] = { 0 };
	
	buf[0] = 0x88;
	GSLDevWriteBuffer(Device, 0xe0, &buf[0], 1);
	wd_delay(5);
	buf[0] = 0x03;
	GSLDevWriteBuffer(Device, 0x80, &buf[0], 1);
	wd_delay(5);
	buf[0] = 0x04;
	GSLDevWriteBuffer(Device, 0xe4, &buf[0], 1);
	wd_delay(5);
	buf[0] = 0x00;
	GSLDevWriteBuffer(Device, 0xe0, &buf[0], 1);
	wd_delay(10);
	return STATUS_SUCCESS;
}

#ifdef GSL9XX_VDDIO_1800
NTSTATUS GSL_IO_CONTROL(IN WDFDEVICE    Device)
{
	UINT8 buf[4] = { 0 };
	UINT8 i;
	wd_delay(10);
	for (i = 0; i < 5; i++){
		buf[0] = 0;
		buf[1] = 0;
		buf[2] = 0xfe;
		buf[3] = 0x1;
		GSLDevWriteBuffer(Device, 0xf0, &buf[0], 4);

		buf[0] = 0x5;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0x80;
		GSLDevWriteBuffer(Device, 0x78, &buf[0], 4);
		wd_delay(5);
	}
	wd_delay(50);
	return STATUS_SUCCESS;
}
//
//NTSTATUS GSL_READ_VDDIO(IN WDFDEVICE    Device)
//{
//	UINT8 buf[4] = { 0 };
//	UINT8 read_buf[4] = { 0 };
//	UINT8 i;
//	wd_delay(30);
//	for (i = 0; i < 5; i++){
//		buf[0] = 0;
//		buf[1] = 0;
//		buf[2] = 0xfe;
//		buf[3] = 0x1;
//		GSLDevWriteBuffer(Device, 0xf0, buf, 4);
//		buf[0] = 0x5;
//		buf[1] = 0;
//		buf[2] = 0;
//		buf[3] = 0x80;
//		if (!NT_SUCCESS(GSLDevReadBuffer(Device, 0x78, read_buf, 4)))
//		{
//			GSLDevReadBuffer(Device, 0x78, read_buf, 4);
//		}
//		DbgPrint("read vddio buf, read_buf[0] = 0x%02x, read_buf[1] = 0x%02x, read_buf[2] = 0x%02x, read_buf[3] = 0x%02x \n", 
//			read_buf[0], read_buf[1], read_buf[2], read_buf[3]);
//		wd_delay(10);
//	}
//	wd_delay(50);
//	return STATUS_SUCCESS;
//}
#endif
NTSTATUS
GSL_Reset_Chip(IN WDFDEVICE    Device)
{
	UCHAR tmp = 0x88;
	UCHAR buf[4] = { 0 };
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SileadTouch GSL_Reset_Chip Entry\n", __FUNCTION__);
	GSLDevWriteBuffer(Device, 0xe0, &tmp, 1);
	wd_delay(10);
	tmp = 0x04;
	GSLDevWriteBuffer(Device, 0xe4, &tmp, 1);
	wd_delay(5);
	GSLDevWriteBuffer(Device, 0xbc, buf, 4);
	wd_delay(5);
	return STATUS_SUCCESS;
}

NTSTATUS
GSL_Startup_Chip(IN WDFDEVICE  Device)
{
	UCHAR tmp = 0x00;
#ifdef GSL9XX_VDDIO_1800
	wd_delay(50);
	GSL_IO_CONTROL(Device);
#endif
	GSLDevWriteBuffer(Device, 0xe0, &tmp, 1);
	wd_delay(10);	
	return STATUS_SUCCESS;
}


#if 0
NTSTATUS
GSL_Load_CFG(IN WDFDEVICE    Device)
{
	//PTS_CFG_DATA ptr_ts_cfg = NULL;
	BYTE addr = 0;
	BYTE buf[4] = {0};
	ULONG source_line = 0;
	ULONG source_len = 0;
	PDEVICE_EXTENSION devContext;

	//BYTE read_buf[4] = { 0 };
	DbgPrintGSL("LOAD -Rrrrrrrr================%s start===============\n",
		__FUNCTION__);

	devContext = GetDeviceContext(Device);
	if (devContext->NoIDVersion)
	{
		gsl_DataInit(gsl_config_data_id);
	}
	wd_delay(20);

	//ptr_ts_cfg = GSL_TS_CFG;	
	source_len = sizeof(GSL_TS_CFG) / sizeof(GSL_TS_CFG[0]);
	
	for (source_line = 0; source_line<source_len; source_line++)
	{
		addr = /*(BYTE)*/(GSL_TS_CFG[source_line].offset);/*
		buf[0] = (BYTE)(GSL_TS_CFG[source_line].val & 0x000000ff);
		buf[1] = (BYTE)((GSL_TS_CFG[source_line].val & 0x0000ff00) >> 8);
		buf[2] = (BYTE)((GSL_TS_CFG[source_line].val & 0x00ff0000) >> 16);
		buf[3] = (BYTE)((GSL_TS_CFG[source_line].val & 0xff000000) >> 24);*/

		buf[0] = (BYTE)(GSL_TS_CFG[source_line].val & 0xff);
		buf[1] = (BYTE)((GSL_TS_CFG[source_line].val >> 8) & 0xff);
		buf[2] = (BYTE)((GSL_TS_CFG[source_line].val >> 16) & 0xff);
		buf[3] = (BYTE)((GSL_TS_CFG[source_line].val >> 24) & 0xff);

		GSLDevWriteBuffer(Device, addr, buf, 4);
	}

	DbgPrintGSL("================%s end===============\n",
		__FUNCTION__);
	return STATUS_SUCCESS;
}
#endif

#if 1
NTSTATUS
GSL_Load_CFG(IN WDFDEVICE    Device)
{
	PTS_CFG_DATA ptr_ts_cfg = NULL;
	BYTE offset = 0;
	BYTE buf[128];
	BYTE send_flag = 0;
	ULONG source_line=0;
	ULONG source_len = 0;
	PDEVICE_EXTENSION devContext;
	
	DbgPrint("================%s start===============\n", __FUNCTION__);

	devContext = GetDeviceContext(Device);
	if(devContext->NoIDVersion)
	{
		gsl_DataInit(gsl_config_data_id);
	}
	
	ptr_ts_cfg = GSL_TS_CFG;	
	source_len = ARRAYSIZE(GSL_TS_CFG);

	for (source_line=0; source_line<source_len; source_line++)
	{
		if(0xf0 == ptr_ts_cfg[source_line].offset)
		{
			buf[0] = (BYTE)(ptr_ts_cfg[source_line].val & 0xff);
			GSLDevWriteBuffer(Device, 0xf0, buf, 1);
			send_flag = 1;
			offset = 0;
		}
		else if(1 == send_flag)
		{
			buf[offset + 0] = (BYTE)(ptr_ts_cfg[source_line].val & 0xff);
			buf[offset + 1] = (BYTE)((ptr_ts_cfg[source_line].val >> 8) & 0xff);
			buf[offset + 2] = (BYTE)((ptr_ts_cfg[source_line].val >> 16) & 0xff);
			buf[offset + 3] = (BYTE)((ptr_ts_cfg[source_line].val >> 24) & 0xff);
			offset += 4;
			if(0 == (offset%I2C_TRANSMIT_LEN))
				GSLDevWriteBuffer(Device, offset - I2C_TRANSMIT_LEN, &buf[offset - I2C_TRANSMIT_LEN], I2C_TRANSMIT_LEN);
			if(offset >= 128)
			{
				send_flag = 0;
			}
		}
	}
	DbgPrint("================%s end===============\n", __FUNCTION__);
	return STATUS_SUCCESS;
}
#endif

UINT32 char_to_uint32(PUCHAR value_char)
{
	UINT32 value = 0;
	UCHAR i;

	for(i = 0; i < 8; i ++)
	{
		if('0' <= value_char[i] && value_char[i] <= '9')
		{
			value += ((UINT32)(value_char[i] - '0'))<<(i*4);
		}
		else if('a' <= value_char[i] && value_char[i] <= 'f')
		{
			value += ((UINT32)(value_char[i] - 'a' + 10))<<(i*4);
		}
		else if('A' <= value_char[i] && value_char[i] <= 'F')
		{
			value += ((UINT32)(value_char[i] - 'A' + 10))<<(i*4);
		}
		else 
			break;
	}

	return value;
}

NTSTATUS
GSL_Load_CFG_Flie(IN WDFDEVICE    Device)
{
	UNICODE_STRING     CFGName;
	OBJECT_ATTRIBUTES  Attributes;
	HANDLE   FileHandle;
	IO_STATUS_BLOCK    ioStatusBlock;
	FILE_STANDARD_INFORMATION StandardInfo;
	LONG BytesRead;
	LONG file_offset;
	PUCHAR file_buf = NULL ;
	UCHAR value_char[8];
	UCHAR i;
	PDEVICE_EXTENSION devContext;
	NTSTATUS Status;
	BYTE offset = 0;
	ULONG id_offset = 0;
	BYTE buf[128];
	BYTE send_flag = 0;
	UINT32 value;
	unsigned int gsl_config_data_id_inner[512] = { 0 };

	RtlInitUnicodeString(&CFGName, L"\\SystemRoot\\System32\\drivers\\SileadTouch.fw");
	InitializeObjectAttributes(&Attributes, &CFGName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	// Do not try to perform any file operations at higher IRQL levels. 
	/*Status = ZwCreateFile(&FileHandle,
		FILE_READ_DATA,  
		&Attributes, 
		&ioStatusBlock,  
		NULL,  
		FILE_ATTRIBUTE_NORMAL,  
		FILE_SHARE_READ,  
		FILE_OPEN,  
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,  
		0 ); */
	Status = ZwOpenFile(&FileHandle, FILE_READ_DATA, &Attributes, &ioStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("GSL_Load_CFG_Flie OpenFile SileadTouch.fw fail! CFGName = %s, Status = 0x%x\n", CFGName, Status);
		return Status;
	}

	Status = ZwQueryInformationFile( FileHandle, 
		&ioStatusBlock, 
		&StandardInfo, 
		sizeof(FILE_STANDARD_INFORMATION), 
		FileStandardInformation ); 
	if(!NT_SUCCESS(Status)) 
	{ 
		DbgPrint ("Error querying info on file %x\n", Status ); 
		ZwClose(FileHandle); 
		return Status; 
	} 

	BytesRead = StandardInfo.EndOfFile.LowPart ;
	file_buf = ExAllocatePool(PagedPool , BytesRead) ;
	if(NULL == file_buf)
	{
		DbgPrint ("allocating memory buffer fail\n" ); 
		ZwClose(FileHandle); 
		return STATUS_UNSUCCESSFUL;
	}

	Status = ZwReadFile(
		FileHandle ,
		NULL , 
		NULL ,
		NULL ,
		&ioStatusBlock ,
		file_buf ,
		BytesRead , 
		NULL ,
		NULL 
		) ;
	if(!NT_SUCCESS(Status)) 
	{ 
		DbgPrint ("read file error %x\n", Status ); 
		ExFreePool(file_buf);
		ZwClose(FileHandle); 
		return Status; 
	} 
	else
		DbgPrintGSL("read file BytesRead = %d\n", BytesRead);
	
	DbgPrint("================%s start===============\n", __FUNCTION__);

	file_offset = 0;
	devContext = GetDeviceContext(Device);
	if(devContext->NoIDVersion)
	{
		while(file_offset < (BytesRead - 1))
		{
			if ((file_buf[file_offset] ^ XOR) == 'i' && (file_buf[file_offset + 1] ^ XOR) == 'd')
				break;
			file_offset++;
		}
		DbgPrint("gsl_config_data_id\n");
		id_offset = 0;
		while(file_offset < BytesRead)
		{
			if((file_buf[file_offset] ^ XOR) == ';')
				break;

			if ((file_buf[file_offset] ^ XOR) == ',')
			{
				for(i = 0; i < 8; i ++ )
				{
					value_char[i] = (file_buf[file_offset - i - 1] ^ XOR);
				}
				value = char_to_uint32(value_char);
				//DbgPrint ("value = 0x%x\n", value ); 
				gsl_config_data_id_inner[id_offset] = value;
				id_offset++;
			}
			file_offset++;
		}
		gsl_DataInit(gsl_config_data_id_inner);
	}

	file_offset = 0;
	while(file_offset < (BytesRead - 2))
	{
		if ((file_buf[file_offset] ^ XOR) == 'C' && (file_buf[file_offset + 1] ^ XOR) == 'F' && (file_buf[file_offset + 2] ^ XOR) == 'G')
			break;
		file_offset++;
	}
	DbgPrint("GSL_TS_CFG\n");
	while(file_offset < (BytesRead - 2))
	{
		if ((file_buf[file_offset] ^ XOR) == ';')
			break;
		
		if(0 == send_flag)
		{
			if ((file_buf[file_offset] ^ XOR) == 'f' && (file_buf[file_offset + 1] ^ XOR) == '0' && (file_buf[file_offset + 2] ^ XOR) == ',')
			{
				send_flag = 1;
			}
		}
		else
		{
			if ((file_buf[file_offset] ^ XOR) == '}' && (file_buf[file_offset + 1] ^ XOR) == ',')
			{
				for(i = 0; i < 8; i ++ )
				{
					value_char[i] = (file_buf[file_offset - i - 1] ^ XOR);
				}
				value = char_to_uint32(value_char);
				//DbgPrint ("value = 0x%x\n", value );
				if(send_flag == 1)
				{
					buf[0] = (char)(value & 0xff);
					GSLDevWriteBuffer(Device, 0xf0, buf, 1);
					offset = 0;
					send_flag = 2;
				}
				else
				{
					buf[offset + 0] = (char)(value & 0xff);
					buf[offset + 1] = (char)((value >> 8) & 0xff);
					buf[offset + 2] = (char)((value >> 16) & 0xff);
					buf[offset + 3] = (char)((value >> 24) & 0xff);
					offset += 4;
					if(0 == (offset%I2C_TRANSMIT_LEN))
						GSLDevWriteBuffer(Device, offset - I2C_TRANSMIT_LEN, &buf[offset - I2C_TRANSMIT_LEN], I2C_TRANSMIT_LEN);
					if(offset >= 128)
					{
						send_flag = 0;
					}
				}
			}
		}
		
		file_offset++;
	}

	ExFreePool(file_buf);
	ZwClose(FileHandle); 
	
	DbgPrint("================%s end===============\n", __FUNCTION__);
	return STATUS_SUCCESS;
}
NTSTATUS
GSL_Init_Chip(IN WDFDEVICE    Device)
{
	NTSTATUS Status = STATUS_SUCCESS;
	Status = GSL_Test_I2C(Device);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("--------GSL Test_I2C fail! Status = 0x%x\n", Status);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "SSSSSSSSSSSSSSSSSSS  GSL_Init_Chip GSL Test_I2C fail! Status = 0x%x\n", Status);
		return Status;
	}
	else
	{
		DbgPrint("++++++++GSL Test_I2C pass! Status = 0x%x\n", Status);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "%s SSSSSSSSSSSSSSSSSSSSS GSL_Init_Chip GSL Test_I2C pass!Status = 0x % x\n", Status);
	}
	GSL_Clr_Reg(Device);
	GSL_Reset_Chip(Device);	
	Status = GSL_Load_CFG_Flie(Device);
	if (!NT_SUCCESS(Status)) 
	{
		DbgPrint("!!!!!!!!!GSL_Load_CFG_Flie fail!!!!!!!!!!!\n");
		GSL_Load_CFG(Device);
	}
	GSL_Startup_Chip(Device);
	GSL_Reset_Chip(Device);
	GSL_Startup_Chip(Device);	
	return STATUS_SUCCESS;
}

NTSTATUS
GSL_Check_Memdata(IN WDFDEVICE    Device)
{
	UCHAR read_buf[4] = { 0 };
	wd_delay(50);
	GSLDevReadBuffer(Device, 0xb0, read_buf, 4);
	DbgPrint("#######%s 0xb0 = %x %x %x %x #######\n", __FUNCTION__, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		GSL_Init_Chip(Device);
	}
	return STATUS_SUCCESS;
}
//
//NTSTATUS
//GSL_Pharse_FW(IN WDFDEVICE    Device)
//{
//	UNICODE_STRING     CFGName;
//	OBJECT_ATTRIBUTES  Attributes;
//	HANDLE   fwFileHandle;
//	IO_STATUS_BLOCK    ioStatusBlock;
//	FILE_STANDARD_INFORMATION StandardInfo;
//	LONG BytesRead;
//	LONG file_offset;
//	PUCHAR file_buf = NULL;
//	UCHAR value_char[8];
//	UCHAR i;
//	PDEVICE_EXTENSION devContext;
//	NTSTATUS Status;
//	BYTE offset = 0;
//	ULONG id_offset = 0;
//	BYTE buf[128];
//	BYTE send_flag = 0;
//	UINT32 value;
//	unsigned int gsl_config_data_id_inner[512] = { 0 };
//
//	RtlInitUnicodeString(&CFGName, L"\\SystemRoot\\System32\\drivers\\sileadtouch.fw");
//	InitializeObjectAttributes(&Attributes, &CFGName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
//
//	// Do not try to perform any file operations at higher IRQL levels. 
//	/*Status = ZwCreateFile(&FileHandle,
//	FILE_READ_DATA,
//	&Attributes,
//	&ioStatusBlock,
//	NULL,
//	FILE_ATTRIBUTE_NORMAL,
//	FILE_SHARE_READ,
//	FILE_OPEN,
//	FILE_SYNCHRONOUS_IO_NONALERT,
//	NULL,
//	0 ); */
//	Status = ZwOpenFile(&fwFileHandle, FILE_READ_DATA, &Attributes, &ioStatusBlock, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
//	if (!NT_SUCCESS(Status)) {
//		DbgPrint("GSL_Pharse_FW ZwOpenFile sileadtouch.fw fail! Status = 0x%x\n", Status);
//		return Status;
//	}
//
//	Status = ZwQueryInformationFile(fwFileHandle,
//		&ioStatusBlock,
//		&StandardInfo,
//		sizeof(FILE_STANDARD_INFORMATION),
//		FileStandardInformation);
//	if (!NT_SUCCESS(Status))
//	{
//		DbgPrint("Error querying info on file %x\n", Status);
//		ZwClose(fwFileHandle);
//		return Status;
//	}
//
//	BytesRead = StandardInfo.EndOfFile.LowPart;
//	file_buf = ExAllocatePool(PagedPool, BytesRead);
//	if (NULL == file_buf)
//	{
//		DbgPrint("allocating memory buffer fail\n");
//		ZwClose(fwFileHandle);
//		return STATUS_UNSUCCESSFUL;
//	}
//
//	Status = ZwReadFile(
//		fwFileHandle,
//		NULL,
//		NULL,
//		NULL,
//		&ioStatusBlock,
//		file_buf,
//		BytesRead,
//		NULL,
//		NULL
//		);
//	if (!NT_SUCCESS(Status))
//	{
//		DbgPrint("read file error %x\n", Status);
//		ExFreePool(file_buf);
//		ZwClose(fwFileHandle);
//		return Status;
//	}
//	else
//		DbgPrintGSL("read file BytesRead = %d\n", BytesRead);
//
//
//}

#ifdef WINKEY_WAKEUP_SYSTEM
NTSTATUS
GSL_Enter_Doze(IN WDFDEVICE    Device)
{
	UCHAR buf[4] = { 0 };
	buf[0] = 0xa;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	GSLDevWriteBuffer(Device, 0xf0, buf, 4);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0x1;
	buf[3] = 0x5a;
	GSLDevWriteBuffer(Device, 0x8, buf, 4);
	wd_delay(10);

	return STATUS_SUCCESS;
}

NTSTATUS
GSL_Exit_Doze(IN WDFDEVICE    Device)
{
	UCHAR buf[4] = { 0 };
	buf[0] = 0xa;
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = 0;
	GSLDevWriteBuffer(Device, 0xf0, buf, 4);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 0x0;
	buf[3] = 0x5a;
	GSLDevWriteBuffer(Device, 0x8, buf, 4);
	wd_delay(10);

	return STATUS_SUCCESS;
}
#endif
