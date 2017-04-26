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

#ifndef _BUTTON_DRIVER_H_
#define _BUTTON_DRIVER_H_

#include <wdm.h>
#include <wdf.h>

NTSTATUS
DriverEntry(
	_In_  PDRIVER_OBJECT   pDriverObject,
	_In_  PUNICODE_STRING  pRegistryPath
);

EVT_WDF_DRIVER_DEVICE_ADD					HidButtonEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP				HidButtonEvtDriverContextCleanup;

#endif