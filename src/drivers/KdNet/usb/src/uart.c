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

//#include <pch.h>
#include <ntddk.h>
#include <stdio.h>
#include <stddef.h>
#include "sunxi.h"
#include <logger.h>

//------------------------------------------------------------------------------
//
// Register addresses
//
#define UART0_BASE		SUNXI_UART0_BASE
#define CCM_BASE		SUNXI_CCM_BASE
#define PIO_BASE		SUNXI_PIO_BASE

/*
 * UART
 */
#define AW_UART_RBR		0x00 /* Receive Buffer Register */
#define AW_UART_THR		0x00 /* Transmit Holding Register */
#define AW_UART_DLL		0x00 /* Divisor Latch Low Register */
#define AW_UART_DLH		0x04 /* Diviso Latch High Register */
#define AW_UART_IER		0x04 /* Interrupt Enable Register */
#define AW_UART_IIR		0x08 /* Interrrupt Identity Register */
#define AW_UART_FCR		0x08 /* FIFO Control Register */
#define AW_UART_LCR		0x0c /* Line Control Register */
#define AW_UART_MCR		0x10 /* Modem Control Register */
#define AW_UART_LSR		0x14 /* Line Status Register */
#define AW_UART_MSR		0x18 /* Modem Status Register */
#define AW_UART_SCH		0x1c /* Scratch Register */
#define AW_UART_USR		0x7c /* Status Register */
#define AW_UART_TFL		0x80 /* Transmit FIFO Level */
#define AW_UART_RFL		0x84 /* RFL */
#define AW_UART_HALT		0xa4 /* Halt TX Register */

//------------------------------------------------------------------------------
static
PUCHAR
s_uartBase;

//------------------------------------------------------------------------------
VOID SunxiUartInit(void)
{

}

void SunxiPutC(const UCHAR byte)
{
    while (!(readb(s_uartBase + AW_UART_USR) & 2));
        writeb(byte, s_uartBase + AW_UART_THR);
}

VOID
DebugOutputInit(
    PDEBUG_DEVICE_DESCRIPTOR pDevice,
    ULONG index
    )
{
    if ((s_uartBase == NULL) && (pDevice != NULL))
       {
       s_uartBase = pDevice->BaseAddress[index].TranslatedAddress;
       }
    if (s_uartBase == NULL) goto Exit;

    SunxiUartInit();

Exit:
    return;
}

//------------------------------------------------------------------------------

VOID
DebugOutputByte(
    const UCHAR byte
    )
{

    // Skip if UART isn't initialized
    if (s_uartBase == NULL) goto Exit;

    SunxiPutC(byte);

Exit:
    return;
}

//------------------------------------------------------------------------------

