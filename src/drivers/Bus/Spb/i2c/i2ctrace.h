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

#ifndef _I2CTRACE_H_
#define _I2CTRACE_H_

extern "C" 
{
//
// Tracing Definitions:
//
// TODO: Define a unique tracing guid.
//
// Control GUID: 
// {3AD0F092-64C8-4e69-B93D-7FB64933FFDD}

#define WPP_CONTROL_GUIDS                           \
    WPP_DEFINE_CONTROL_GUID(                        \
        PbcTraceGuid,                               \
        (3AD0F092,64C8,4e69,B93D,7FB64933FFDD),     \
        WPP_DEFINE_BIT(TRACE_FLAG_WDFLOADING)       \
        WPP_DEFINE_BIT(TRACE_FLAG_SPBDDI)           \
        WPP_DEFINE_BIT(TRACE_FLAG_PBCLOADING)       \
        WPP_DEFINE_BIT(TRACE_FLAG_TRANSFER)         \
        WPP_DEFINE_BIT(TRACE_FLAG_OTHER)            \
        )
}

#define WPP_LEVEL_FLAGS_LOGGER(level,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

// begin_wpp config
// FUNC FuncEntry{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// FUNC FuncExit{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// USEPREFIX(FuncEntry, "%!STDPREFIX! [%!FUNC!] --> entry");
// USEPREFIX(FuncExit, "%!STDPREFIX! [%!FUNC!] <--");
// end_wpp

#endif // _I2CTRACE_H_
