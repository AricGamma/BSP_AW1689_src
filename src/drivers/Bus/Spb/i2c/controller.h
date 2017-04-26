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

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

//
// Controller specific function prototypes.
//

VOID ControllerInitialize(
    _In_  PPBC_DEVICE   pDevice);

VOID ControllerUninitialize(
    _In_  PPBC_DEVICE   pDevice);

VOID
ControllerConfigureForTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest);

//NTSTATUS
//ControllerTransferData(
//    _In_  PPBC_DEVICE   pDevice,
//    _In_  PPBC_REQUEST  pRequest);
    
VOID
ControllerCompleteTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest);

VOID
ControllerEnableInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask);

VOID
ControllerDisableInterrupts(
    _In_  PPBC_DEVICE   pDevice);

ULONG
ControllerGetInterruptStatus(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask);

//VOID
//ControllerAcknowledgeInterrupts(
//    _In_  PPBC_DEVICE   pDevice,
//    _In_  ULONG         InterruptMask);

VOID
ControllerProcessInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest,
    _In_  ULONG         InterruptStatus);

#endif
