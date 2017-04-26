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

//
// Query/set value under software key - Regs added under [.NT] installation section of INF
//

//
// Query/set value under hardware Device Parameters key - Regs added under [.NT.HW] installation section of INF
//
NTSTATUS RegQueryDeviceDwordValue(_In_ WDFDEVICE pDevice, _In_ PCWSTR pValueName, _Out_ ULONG *pValue);
NTSTATUS RegSetDeviceDwordValue(_In_ WDFDEVICE pDevice, _In_ PCWSTR pValueName, _In_ ULONG Value);

//
// Query/set value under service Parameters key - Regs added under [.NT.Services] installation section of INF
//