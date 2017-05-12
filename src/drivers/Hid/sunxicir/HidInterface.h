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

#pragma once
#include "Driver.h"
#include "Device.h"
#include "Common.h"

// {EB036691-9915-4B73-B66B-E5EB9500CE61}
DEFINE_GUID(GUID_DEVCLASS_HIDINJECTOR,
	0xeb036691, 0x9915, 0x4b73, 0xb6, 0x6b, 0xe5, 0xeb, 0x95, 0x0, 0xce, 0x61);

// {9EDB43C3-28F9-4111-98F9-0CBB390241BE}
DEFINE_GUID(GUID_DEVINTERFACE_HIDINJECTOR,
	0x9edb43c3, 0x28f9, 0x4111, 0x98, 0xf9, 0xc, 0xbb, 0x39, 0x2, 0x41, 0xbe);


// {0B438283-FBAE-44CC-8958-D4F00722CB46}
DEFINE_GUID(GUID_BUS_HIDINJECTOR,
	0xb438283, 0xfbae, 0x44cc, 0x89, 0x58, 0xd4, 0xf0, 0x7, 0x22, 0xcb, 0x46);


NTSTATUS RegisterVhidReadyNotification(_In_ WDFDEVICE Device);


NTSTATUS VhidReadyNotificationCallback(_In_ PVOID pDeviceChange, _In_ PVOID pContext);



