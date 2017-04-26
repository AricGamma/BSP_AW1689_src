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

#include "Device.h"

//
// TODO: Change the following info to DMA HW config info
//
#define DMA_CODEC_FIFO_TX__ADDRESS 0x01c22c20
#define DMA_CODEC_FIFO_RX__ADDRESS 0x01c22c10
#define DMA_TX_OFFSET 0x00
#define DMA_RX_OFFSET 0x00

#define DMA_STATE_NONE 0x00
#define DMA_STATE_TRANSMIT_CONFIGED    0x00000001
#define DMA_STATE_RECEIVE_CONFIGED     0x00000002
#define DMA_STATE_TRANSMIT_STARTED     0x00000004
#define DMA_STATE_RECEIVE_STARTED      0x00000008
#define DMA_STATE_TRANSMIT_CANCELLED   0x00000010
#define DMA_STATE_RECEIVE_CANCELLED    0x00000020

#define StateAdd(_flag_, _state_) (_flag_ |= _state_)
#define StateRemove(_flag_, _state_) (_flag_ &= ~_state_)
#define StateTest(_flag_, _state_) ((_flag_ & _state_) == _state_)

NTSTATUS I2SDmaInitialize(_In_ PDEVICE_CONTEXT pDevExt);
NTSTATUS I2SDmaTransmit(_In_ PDEVICE_CONTEXT pDevExt, _In_ PMDL pTransferMdl, _In_ ULONG TransferSize, _In_ ULONG NotificationsPerBuffer);
NTSTATUS I2SDmaReceive(_In_ PDEVICE_CONTEXT pDevExt, _In_ PMDL pTransferMdl, _In_ ULONG TransferSize, _In_ ULONG NotificationsPerBuffer);
NTSTATUS I2SDmaStop(_In_ PDEVICE_CONTEXT pDevExt, _In_ BOOL IsCapture);