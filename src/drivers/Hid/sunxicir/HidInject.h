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
#include "Common.h"
#include "WinVK.h"
#include <minwindef.h>

UCHAR VKeyToKeyboardUsage(UCHAR vk);
UCHAR ScanCodeToKeyboardUsage(UCHAR code);
UCHAR UnicodeToKeyboardUsage(WCHAR ch);

// No usage is defined for this VKEY
#define USAGE_NONE 0

// A usage may be defined, but we haven't bothered to find it yet.  
#define USAGE_TODO 0

BOOL SetKeybaordUsage(HIDINJECTOR_INPUT_REPORT *Rep, UCHAR Usage);

BOOL ClearKeyboardUsage(HIDINJECTOR_INPUT_REPORT *Rep, UCHAR Usage);
