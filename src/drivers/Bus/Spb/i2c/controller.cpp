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

#include "internal.h"
#include "controller.h"
#include "device.h"

#include "controller.tmh"

const PBC_TRANSFER_SETTINGS g_TransferSettings[] = 
{
    // TODO: Update this array to reflect changes
    //       made to the PBC_TRANSFER_SETTINGS
    //       structure in internal.h.

    // Bus condition        IsStart  IsEnd
    {BusConditionDontCare,  FALSE,   FALSE}, // SpbRequestTypeInvalid
    {BusConditionFree,      TRUE,    TRUE},  // SpbRequestTypeSingle
    {BusConditionFree,      TRUE,    FALSE}, // SpbRequestTypeFirst
    {BusConditionBusy,      FALSE,   FALSE}, // SpbRequestTypeContinue
    {BusConditionBusy,      FALSE,   TRUE}   // SpbRequestTypeLast
};

VOID TwiDelay (
	_In_ LONG msec
	)
{
	do{
		__nop();
	} while (--msec > 0);
}


inline VOID 
TwiClearIrqFlag (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal =pDevice->pRegisters->CtlReg.Read();
	
	RegVal |= TWI_CTL_INTFLG;	/* start and stop bit should be 0 */
	RegVal &= ~(TWI_CTL_STA | TWI_CTL_STP);
	pDevice->pRegisters->CtlReg.Write(RegVal);

	/* read two more times to make sure that interrupt flag does really be cleared */
	{
		ULONG temp;
		temp = pDevice->pRegisters->CtlReg.Read();
		temp |= pDevice->pRegisters->CtlReg.Read();
	}
}

/* get data first, then clear flag */
inline VOID 
TwiGetByte (
	_In_  PPBC_DEVICE  pDevice, 
	_In_  UCHAR  *buffer
	)
{
	*buffer = (UCHAR) (TWI_DATA_MASK & pDevice->pRegisters->DatarReg.Read());
	TwiClearIrqFlag(pDevice);
}

/* only get data, we will clear the flag when stop */
inline VOID 
TwiGetLastByte (
	_In_  PPBC_DEVICE  pDevice, 
	_In_  UCHAR  *buffer
	)
{
	*buffer = (UCHAR) (TWI_DATA_MASK & pDevice->pRegisters->DatarReg.Read());
}

/* write data and clear irq flag to trigger send flow */
inline VOID 
TwiPutByte (
	_In_  PPBC_DEVICE  pDevice, 
	_In_  UCHAR *buffer
	)
{
	pDevice->pRegisters->DatarReg.Write(*buffer);
	TwiClearIrqFlag(pDevice);
}

inline VOID 
TwiEnableIrq (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();

	/*
	* 1 when enable irq for next operation, set intflag to 0 to prevent to clear it by a mistake
	*   (intflag bit is write-1-to-clear bit)
	* 2 Similarly, mask START bit and STOP bit to prevent to set it twice by a mistake
	*   (START bit and STOP bit are self-clear-to-0 bits)
	*/
	RegVal |= TWI_CTL_INTEN;
	RegVal &= ~(TWI_CTL_STA | TWI_CTL_STP | TWI_CTL_INTFLG);
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

inline VOID 
TwiDisableIrq (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal &= ~TWI_CTL_INTEN;
	RegVal &= ~(TWI_CTL_STA | TWI_CTL_STP | TWI_CTL_INTFLG);
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

VOID
TwiWriteClock (
	_In_  PPBC_DEVICE  pDevice,
	_In_  ULONG clk_n,
	_In_  ULONG clk_m
	)
{
	ULONG reg_val = pDevice->pRegisters->ClkReg.Read();
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%s: clk_n = %d, clk_m = %d\n", __FUNCTION__, clk_n, clk_m));
	reg_val &= ~(TWI_CLK_DIV_M | TWI_CLK_DIV_N);
	reg_val |= (clk_n | (clk_m << 3));
	pDevice->pRegisters->ClkReg.Write(reg_val);

}

/* trigger start signal, the start bit will be cleared automatically */
inline VOID 
TwiSetStart (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal |= TWI_CTL_STA;
	RegVal &= ~TWI_CTL_INTFLG;
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

/* get start bit status, poll if start signal is sent */
inline ULONG 
TwiGetStart (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal >>= 5;
	return RegVal & 1;
}

/* trigger stop signal, the stop bit will be cleared automatically */
inline VOID 
TwiSetStop (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal |= TWI_CTL_STP;
	RegVal &= ~TWI_CTL_INTFLG;
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

/* get stop bit status, poll if stop signal is sent */
inline ULONG 
TwiGetStop (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal >>= 4;
	return RegVal & 1;
}

inline VOID 
TwiDisableAck (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal &= ~TWI_CTL_ACK;
	RegVal &= ~TWI_CTL_INTFLG;
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

/* when sending ack or nack, it will send ack automatically */
inline VOID 
TwiEnableAck (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal |= TWI_CTL_ACK;
	RegVal &= ~TWI_CTL_INTFLG;
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

/* get the interrupt flag */
inline ULONG 
TwiQueryIrqFlag (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	return (RegVal & TWI_CTL_INTFLG);//0x 0000_1000
}

/* get interrupt status */
inline ULONG 
TwiQueryIrqStatus (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->StatReg.Read();
	return (RegVal & TWI_STAT_MASK);
}

VOID
TwiSetClock(
	_In_  PPBC_DEVICE  pDevice,
	_In_  ULONG Clock,
	_In_  ULONG sclk_req
	)
/*
	Routine Description:

		This routine set the controller clock.

	Arguments:

		pDevice - a pointer to the PBC device context

  Return Value:

		None.
*/
{
	ULONG clk_m = 0;
	ULONG clk_n = 0;
	ULONG _2_pow_clk_n = 1;
	ULONG src_clk = Clock / 10;
	ULONG divider = src_clk / sclk_req;  // 400khz or 100khz
	ULONG sclk_real = 0;      // the real clock frequency

	if (divider == 0) {
		clk_m = 1;
		goto set_clk;
	}

	// search clk_n and clk_m,from large to small value so that can quickly find suitable m & n. 
	while (clk_n < 8) { // 3bits max value is 8
		/* (m+1)*2^n = divider -->m = divider/2^n -1 */
		clk_m = (divider / _2_pow_clk_n) - 1;
		/* clk_m = (divider >> (_2_pow_clk_n>>1))-1 */
		while (clk_m < 16) { /* 4bits max value is 16 */
			sclk_real = src_clk / (clk_m + 1) / _2_pow_clk_n;  /* src_clk/((m+1)*2^n) */
			if (sclk_real <= sclk_req) {
				goto set_clk;
			}
			else {
				clk_m++;
			}
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* mutilple by 2 */
	}

set_clk:
	TwiWriteClock(pDevice, clk_n, clk_m);

}

inline VOID 
TwiSoftReset(
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->SrstReg.Read();
	RegVal |= TWI_SRST_SRST;
	pDevice->pRegisters->SrstReg.Write(RegVal);
}

inline VOID 
TwiDisableBus(
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal &= ~TWI_CTL_BUSEN;
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

inline VOID 
TwiEnableBus(
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG RegVal = pDevice->pRegisters->CtlReg.Read();
	RegVal |= TWI_CTL_BUSEN;
	pDevice->pRegisters->CtlReg.Write(RegVal);
}

/* Enhanced Feature Register */
inline VOID 
TwiSetEfr(
	_In_  PPBC_DEVICE  pDevice,
	_In_  ULONG efr)
{
	ULONG RegVal = pDevice->pRegisters->EfrReg.Read();

	RegVal &= ~TWI_EFR_MASK;
	efr &= TWI_EFR_MASK;
	RegVal |= efr;
	pDevice->pRegisters->EfrReg.Write(RegVal);
}


/* function  */
int 
TwiStart (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG timeout = 0xff;

	TwiSetStart(pDevice);
	while ((1 == TwiGetStart(pDevice)) && (--timeout));
	if (timeout == 0) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c] START can't sendout!\n"));
		return AW_I2C_FAIL;
	}

	return AW_I2C_OK;
}

int 
TwiRestart (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG timeout = 0xff;
	TwiSetStart(pDevice);
	TwiClearIrqFlag(pDevice);
	while ((1 == TwiGetStart(pDevice)) && (--timeout));
	if (timeout == 0) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c] Restart can't sendout!\n"));
		return AW_I2C_FAIL;
	}
	return AW_I2C_OK;
}

int 
TwiStop (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG timeout = 0xff;

	TwiSetStop(pDevice);
	TwiClearIrqFlag(pDevice);

	TwiGetStop(pDevice);/* it must delay 1 nop to check stop bit */
	while ((1 == TwiGetStop(pDevice)) && (--timeout));
	if (timeout == 0) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c] STOP can't sendout!\n"));
		return AW_I2C_TFAIL;
	}

	timeout = 0xff;
	while ((TWI_STAT_IDLE != pDevice->pRegisters->StatReg.Read()) && (--timeout));
	if (timeout == 0) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c] i2c state isn't idle(0xf8)\n"));
		return AW_I2C_TFAIL;
	}

	timeout = 0xff;
	while ((TWI_LCR_IDLE_STATUS != pDevice->pRegisters->LcrReg.Read()) && (--timeout));
	if (timeout == 0) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c] i2c lcr isn't idle(0x3a)\n"));
		return AW_I2C_TFAIL;
	}

	return AW_I2C_OK;
}

/* get SDA state */
ULONG 
TwiGetSda (
	_In_  PPBC_DEVICE  pDevice
	)
{
	ULONG status = 0;
	status = TWI_LCR_SDA_STATE_MASK & pDevice->pRegisters->LcrReg.Read();
	status >>= 4;
	return  (status & 0x1);
}

/* set SCL level(high/low), only when SCL enable */
VOID 
TwiSetScl ( 
	_In_  PPBC_DEVICE  pDevice, 
	_In_  ULONG HiLo)
{
	ULONG RegVal = pDevice->pRegisters->LcrReg.Read();
	RegVal &= ~TWI_LCR_SCL_CTL;
	HiLo &= 0x01;
	RegVal |= (HiLo << 3);
	pDevice->pRegisters->LcrReg.Write(RegVal);
}

/* enable SDA or SCL */
VOID 
TwiEnableLcr (
	_In_  PPBC_DEVICE  pDevice,
	_In_  ULONG SdaScl)
{
	ULONG RegVal = pDevice->pRegisters->LcrReg.Read();
	SdaScl &= 0x01;
	if (SdaScl)
		RegVal |= TWI_LCR_SCL_EN;/* enable scl line control */
	else
		RegVal |= TWI_LCR_SDA_EN;/* enable sda line control */

	pDevice->pRegisters->LcrReg.Write(RegVal);
}

/* disable SDA or SCL */
VOID 
TwiDisableLcr (
	_In_  PPBC_DEVICE  pDevice, 
	_In_  ULONG  SdaScl)
{
	ULONG RegVal = pDevice->pRegisters->LcrReg.Read();
	SdaScl &= 0x01;
	if (SdaScl)
		RegVal &= ~TWI_LCR_SCL_EN;/* disable scl line control */
	else
		RegVal &= ~TWI_LCR_SDA_EN;/* disable sda line control */

	pDevice->pRegisters->LcrReg.Write(RegVal);
}


/* send 9 clock to release sda */
int 
TwiSendClk9pulse(
	_In_  PPBC_DEVICE  pDevice
	)
{
	int TwiScl = 1;
	int low = 0;
	int high = 1;
	int cycle = 0;

	/* enable scl control */
	TwiEnableLcr(pDevice, TwiScl);

	while (cycle < 10)
	{
		if (TwiGetSda(pDevice)
			&& TwiGetSda(pDevice)
			&& TwiGetSda(pDevice)) {
			break;
		}
		/* twi_scl -> low */
		TwiSetScl(pDevice, low);
		TwiDelay(1000);

		/* twi_scl -> high */
		TwiSetScl(pDevice, high);
		TwiDelay(1000);
		cycle++;
	}

	if (TwiGetSda(pDevice)) {
		TwiDisableLcr(pDevice, TwiScl);
		return AW_I2C_OK;
	}
	else {
		KdPrintEx((DPFLTR_IHVAUDIO_ID, DPFLTR_INFO_LEVEL, "[i2c] SDA is still Stuck Low, failed. \n"));
		TwiDisableLcr(pDevice, TwiScl);
		return AW_I2C_FAIL;
	}
}


VOID 
TwiI2cAddrByte (
	_In_  PPBC_DEVICE  pDevice
	)
{
	UCHAR addr = 0;
	UCHAR tmp = 0;

	if (pDevice->pCurrentTarget->Settings.AddressMode == AddressMode10Bit) {
		/* 0111_10xx,ten bits address--9:8bits */
		tmp = 0x78 | (((pDevice->pCurrentTarget->Settings.Address) >> 8) & 0x03);
		addr = tmp << 1;//1111_0xx0
		/* how about the second part of ten bits addr? Answer: deal at twi_core_process() */
	}
	else {
		/* 7-1bits addr, xxxx_xxx0 */
		addr = (pDevice->pCurrentTarget->Settings.Address & 0x7f) << 1;
	}

	/* read, default value is write */
	if (pDevice->pCurrentTarget->pCurrentRequest->Direction & SpbTransferDirectionFromDevice) {
		addr |= 1;
	}

#if 0
	if (i2c->bus_num == CONFIG_SUN6I_I2C_PRINT_TRANSFER_INFO_WITH_BUS_NUM) {
		if (i2c->msg[i2c->msg_idx].flags & I2C_M_TEN) {
			I2C_DBG("[i2c%d] first part of 10bits = 0x%x\n", i2c->bus_num, addr);
		}
		I2C_DBG("[i2c%d] 7bits+r/w = 0x%x\n", i2c->bus_num, addr);
	}
#else

	if (pDevice->pCurrentTarget->Settings.AddressMode == AddressMode10Bit)  {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c%d] first part of 10bits = 0x%x\n", pDevice->BusNumber, pDevice->pCurrentTarget->Settings.Address));
	}

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c] 7bits+r/w = 0x%x\n", addr));

#endif
	/* send 7bits+r/w or the first part of 10bits */
	TwiPutByte(pDevice, &addr);
}


VOID
ControllerInitialize(
    _In_  PPBC_DEVICE  pDevice
    )
/*++
 
  Routine Description:

    This routine initializes the controller hardware.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_PBCLOADING);
                        
    NT_ASSERT(pDevice != NULL);

	//
	//twi enable bus
	//
	TwiEnableBus(pDevice);

	//
	//set i2c clock && speed
	//
	if (pDevice->BusSpeed)
		TwiSetClock(pDevice, 24000000, pDevice->BusSpeed);
	else
		TwiSetClock(pDevice, 24000000, 200000);
	
	//
	//restart the twi
	//
	TwiSoftReset(pDevice);


	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "TWI_CTL_REG(0x0c):  0x%08x \n", pDevice->pRegisters->CtlReg.Read()));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "TWI_STAT_REG(0x10): 0x%08x \n", pDevice->pRegisters->StatReg.Read()));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "TWI_CLK_REG(0x14):  0x%08x \n", pDevice->pRegisters->ClkReg.Read()));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "TWI_SRST_REG(0x18): 0x%08x \n", pDevice->pRegisters->SrstReg.Read()));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "TWI_EFR_REG(0x1c):  0x%08x \n", pDevice->pRegisters->EfrReg.Read()));



    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_PBCLOADING);
}

VOID
ControllerUninitialize(
    _In_  PPBC_DEVICE  pDevice
    )
/*++
 
  Routine Description:

    This routine uninitializes the controller hardware.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_PBCLOADING);

    NT_ASSERT(pDevice != NULL);
	
	//
	//disable twi bus
	//
	TwiDisableBus(pDevice);

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_PBCLOADING);
}

VOID
ControllerConfigureForTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest
    )
/*++
 
  Routine Description:

    This routine configures and starts the controller
    for a transfer.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pRequest != NULL);

    pRequest->Settings = g_TransferSettings[pRequest->SequencePosition];
    pRequest->Status = STATUS_SUCCESS;
	int ret = -1;

	if ((pRequest->SequencePosition == SpbRequestSequencePositionSingle) ||
		(pRequest->SequencePosition == SpbRequestSequencePositionFirst)) {

			TwiSoftReset(pDevice);
			TwiDelay(50);

			while (TWI_STAT_IDLE != TwiQueryIrqStatus(pDevice) &&
				TWI_STAT_BUS_ERR != TwiQueryIrqStatus(pDevice) &&
				TWI_STAT_ARBLOST_SLAR_ACK != TwiQueryIrqStatus(pDevice)) {
				KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c%d] bus is busy, status = %x\n", pDevice->BusNumber, TwiQueryIrqStatus(pDevice)));
				if (AW_I2C_OK == TwiSendClk9pulse(pDevice)) {
					break;
				}
				else {
					ret = AW_I2C_RETRY;
					goto Out;
				}
			}

			TwiEnableIrq(pDevice);  /* enable irq */
			TwiDisableAck(pDevice); /* disabe ACK */
			TwiSetEfr(pDevice, 0);

			//set speed
			if (pDevice->pCurrentTarget->Settings.ConnectionSpeed != 200000) {
				TwiSetClock(pDevice, 24000000, pDevice->pCurrentTarget->Settings.ConnectionSpeed);
			}

			ret = TwiStart(pDevice);
			if (ret == AW_I2C_FAIL) {
				TwiSoftReset(pDevice);
				TwiDisableIrq(pDevice);  /* disable irq */
				pDevice->Status = I2C_XFER_IDLE;
				ret = AW_I2C_RETRY;
				goto Out;
			}

	}
	else if ((pRequest->SequencePosition == SpbRequestSequencePositionContinue) ||
		(pRequest->SequencePosition == SpbRequestSequencePositionLast)) {

			ret = TwiRestart(pDevice);
			if (ret == AW_I2C_FAIL) {/* START fail */
				goto Out;
		}
	}

    if (pRequest->Settings.IsStart)
    {
        // TODO: Perform any action to program a start bit.
    }
    else if (pRequest->Settings.IsEnd)
    {
        // TODO: Perform any action to program a stop bit.
    }

    if (pRequest->Direction == SpbTransferDirectionToDevice)
    {
        // TODO: Perform write-specific configuration,
        //       i.e. pRequest->DataReadyFlag = ...
    }
    else if (pRequest->Direction == SpbTransferDirectionFromDevice)
    {
        // TODO: Perform read-specific configuration,
        //       i.e. pRequest->DataReadyFlag = ...
    }

    pDevice->InterruptStatus = 0;

    Trace(
        TRACE_LEVEL_VERBOSE,
        TRACE_FLAG_TRANSFER,
        "Controller configured for %s of %Iu bytes to address 0x%lx "
        "(SPBREQUEST %p, WDFDEVICE %p)",
        pRequest->Direction == SpbTransferDirectionFromDevice ? "read" : "write",
        pRequest->Length,
        pDevice->pCurrentTarget->Settings.Address,
        pRequest->SpbRequest,
        pDevice->FxDevice);


    ControllerEnableInterrupts(
        pDevice, 
        PbcDeviceGetInterruptMask(pDevice));
Out:

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
ControllerProcessInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest,
    _In_  ULONG         InterruptStatus
    )
/*++
 
  Routine Description:

    This routine processes a hardware interrupt. Activities
    include checking for errors and transferring data.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context
    InterruptStatus - saved interrupt status bits from the ISR.
        These have already been acknowledged and disabled

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

	int ErrCode = 0;
	UCHAR tmp = 0;
	ULONG state = 0;
	UCHAR buffer[256] = {0};

	//AWPRINT("ControllerProcessInterrupts");

	state = TwiQueryIrqStatus(pDevice);
	//KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[i2c%d]: Address:0x%2x, state:0x%2x\n",
	//			pDevice->BusNumber, pDevice->pCurrentTarget->Settings.Address,state));

    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pRequest != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Ready to process interrupts with status 0x%lx for WDFDEVICE %p",
        InterruptStatus,
        pDevice->FxDevice);

	pRequest->Status = STATUS_SUCCESS;

	switch (state)
	{
	case TWI_STAT_IDLE: /* (0xf8)On reset or stop the bus is idle, use only at poll method */
		ErrCode = 0xf8;
		goto ErrOut;
	case TWI_STAT_TX_STA: /* A START condition has been transmitted */
	case TWI_STAT_TX_RESTA: /* A repeated start condition has been transmitted */
		TwiI2cAddrByte(pDevice);/* send slave address */
		break;
	case TWI_STAT_TX_SAW_NAK: /* second addr has transmitted, ACK not received!    */
	case TWI_STAT_TX_AW_NAK: /* SLA+W has been transmitted; NOT ACK has been received */
		ErrCode = 0x20;
		pRequest->Status = STATUS_NO_SUCH_DEVICE;
		goto ErrOut;
	case TWI_STAT_TX_AW_ACK: /* SLA+W has been transmitted; ACK has been received */
		/* if any, send second part of 10 bits addr */
		if (pDevice->pCurrentTarget->Settings.AddressMode & AddressMode10Bit) {
			tmp = pDevice->pCurrentTarget->Settings.Address & 0xff;  /* the remaining 8 bits of address */
			TwiPutByte(pDevice, &tmp); /* case 0xd0: */
			break;
		}
		/* for 7 bit addr, then directly send data byte--case 0xd0:  */
	case TWI_STAT_TX_SAW_ACK: /* second addr has transmitted,ACK received!     */
	case TWI_STAT_TXD_ACK: /* Data byte in DATA REG has been transmitted; ACK has been received */
		/* after send register address then START send write data  */
		if (pDevice->pCurrentTarget->pCurrentRequest->Information < pDevice->pCurrentTarget->pCurrentRequest->Length) {
			//GetRequestByte(pDevice, buffer);
			PbcRequestGetByte(pRequest, pDevice->pCurrentTarget->pCurrentRequest->Information, buffer);
			TwiPutByte(pDevice, buffer);
			pDevice->pCurrentTarget->pCurrentRequest->Information++;
			break;
		}
		if ((pDevice->pCurrentTarget->pCurrentRequest->TransferIndex + 1) < pDevice->pCurrentTarget->pCurrentRequest->TransferCount)
			ControllerCompleteTransfer(pDevice, pRequest);
		else
			goto ErrOut;
		break; 

	case 0x30: /* Data byte in I2CDAT has been transmitted; NOT ACK has been received */
		ErrCode = 0x30;//err,wakeup the thread
		pRequest->Status = STATUS_UNSUCCESSFUL;
		goto ErrOut;
	case 0x38: /* Arbitration lost during SLA+W, SLA+R or data bytes */
		ErrCode = 0x38;//err,wakeup the thread
		pRequest->Status = STATUS_UNSUCCESSFUL;
		goto ErrOut;
	case 0x40: /* SLA+R has been transmitted; ACK has been received */
		/* with Restart,needn't to send second part of 10 bits addr,refer-"I2C-SPEC v2.1" */
		/* enable A_ACK need it(receive data len) more than 1. */
		if (pDevice->pCurrentTarget->pCurrentRequest->Length > 1) {
			/* send register addr complete,then enable the A_ACK and get ready for receiving data */
			TwiEnableAck(pDevice);
			TwiClearIrqFlag(pDevice);/* jump to case 0x50 */
		}
		else if (pDevice->pCurrentTarget->pCurrentRequest->Length == 1) {
			TwiClearIrqFlag(pDevice);/* jump to case 0x58 */
		}
		break;
	case 0x48: /* SLA+R has been transmitted; NOT ACK has been received */
		ErrCode = 0x48;//err,wakeup the thread
		pRequest->Status = STATUS_UNSUCCESSFUL;
		goto ErrOut;
	case 0x50: /* Data bytes has been received; ACK has been transmitted */
		/* receive first data byte */
		if (pDevice->pCurrentTarget->pCurrentRequest->Information < pDevice->pCurrentTarget->pCurrentRequest->Length) {
			/* more than 2 bytes, the last byte need not to send ACK */
			if ((pDevice->pCurrentTarget->pCurrentRequest->Information + 2) == pDevice->pCurrentTarget->pCurrentRequest->Length) {
				TwiDisableAck(pDevice);/* last byte no ACK */
			}
			/* get data then clear flag,then next data comming */
			TwiGetByte(pDevice, buffer);
			//SetRequestByte(pDevice, buffer);
			PbcRequestSetByte(pRequest, pDevice->pCurrentTarget->pCurrentRequest->Information, *buffer);
			pDevice->pCurrentTarget->pCurrentRequest->Information++;
			break;
		}
		/* err process, the last byte should be @case 0x58 */
		ErrCode = AW_I2C_FAIL;/* err, wakeup */
		pRequest->Status = STATUS_UNSUCCESSFUL;
		goto ErrOut;
	case 0x58: /* Data byte has been received; NOT ACK has been transmitted */
		/* received the last byte  */
		if (pDevice->pCurrentTarget->pCurrentRequest->Information == pDevice->pCurrentTarget->pCurrentRequest->Length - 1) {
			TwiGetLastByte(pDevice, buffer);
			PbcRequestSetByte(pRequest, pDevice->pCurrentTarget->pCurrentRequest->Information, *buffer);
			//SetRequestByte(pDevice, buffer);
			pDevice->pCurrentTarget->pCurrentRequest->Information++;
			if ((pDevice->pCurrentTarget->pCurrentRequest->TransferIndex + 1) < pDevice->pCurrentTarget->pCurrentRequest->TransferCount)
				ControllerCompleteTransfer(pDevice, pRequest);
			else if ((pDevice->pCurrentTarget->pCurrentRequest->TransferIndex + 1) == pDevice->pCurrentTarget->pCurrentRequest->TransferCount) {
				ErrCode = AW_I2C_OK; // succeed,wakeup the thread
				goto ErrOut;
			}
			break;
		}
		else {
			ErrCode = 0x58;
			pRequest->Status = STATUS_UNSUCCESSFUL;
			goto ErrOut;
		}
	case 0x00: /* Bus error during master or slave mode due to illegal level condition */
		ErrCode = 0xff;
		pRequest->Status = STATUS_UNSUCCESSFUL;
		goto ErrOut;
	default:
		ErrCode = state;
		pRequest->Status = STATUS_UNSUCCESSFUL;
		goto ErrOut;
	}
	return ;
ErrOut :
	if (AW_I2C_TFAIL == TwiStop(pDevice)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[i2c] STOP failed!\n"));
	}
		ControllerCompleteTransfer(pDevice, pRequest);
		PbcRequestComplete(pRequest);

    FuncExit(TRACE_FLAG_TRANSFER);
}

//NTSTATUS
//ControllerTransferData(
//    _In_  PPBC_DEVICE   pDevice,
//    _In_  PPBC_REQUEST  pRequest
//    )
///*++
// 
//  Routine Description:
//
//    This routine transfers data to or from the device.
//
//  Arguments:
//
//    pDevice - a pointer to the PBC device context
//    pRequest - a pointer to the PBC request context
//
//  Return Value:
//
//    None.
//
//--*/
//{
//    FuncEntry(TRACE_FLAG_TRANSFER);
//
//    UNREFERENCED_PARAMETER(pDevice);
//
//    size_t bytesToTransfer = 0;
//    NTSTATUS status = STATUS_SUCCESS;
//
//    //
//    // Write
//    //
//
//    if (pRequest->Direction == SpbTransferDirectionToDevice)
//    {
//        Trace(
//            TRACE_LEVEL_INFORMATION, 
//            TRACE_FLAG_TRANSFER,
//            "Ready to write %Iu byte(s) for address 0x%lx", 
//            bytesToTransfer,
//            pDevice->pCurrentTarget->Settings.Address);
//
//        // TODO: Perform write. May need to use 
//        //       PbcRequestGetByte() or some variation.
//    }
//
//    //
//    // Read
//    //
//
//    else
//    {
//
//        Trace(
//            TRACE_LEVEL_INFORMATION, 
//            TRACE_FLAG_TRANSFER,
//            "Ready to read %Iu byte(s) for address 0x%lx", 
//            bytesToTransfer,
//            pDevice->pCurrentTarget->Settings.Address);
//
//        // TODO: Perform read. May need to use 
//		//PbcRequestSetByte();
//    }
//
//    //
//    // Update request context with bytes transferred.
//    //
//
//    pRequest->Information += bytesToTransfer;
//
//    FuncExit(TRACE_FLAG_TRANSFER);
//    
//    return status;
//}

VOID
ControllerCompleteTransfer(
    _In_  PPBC_DEVICE   pDevice,
    _In_  PPBC_REQUEST  pRequest
    )
/*++
 
  Routine Description:

    This routine completes a data transfer. Unless there are
    more transfers remaining in the sequence, the request is
    completed.

  Arguments:

    pDevice - a pointer to the PBC device context
    pRequest - a pointer to the PBC request context

  Return Value:

    None. The request is completed asynchronously.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice  != NULL);
    NT_ASSERT(pRequest != NULL);

    Trace(
        TRACE_LEVEL_INFORMATION,
        TRACE_FLAG_TRANSFER,
        "Transfer (index %lu) %s with %Iu bytes for address 0x%lx "
        "(SPBREQUEST %p)",
        pRequest->TransferIndex,
        NT_SUCCESS(pRequest->Status) ? "complete" : "error",
        pRequest->Information,
        pDevice->pCurrentTarget->Settings.Address,
        pRequest->SpbRequest);

    //
    // Update request context with information from this transfer.
    //

    pRequest->TotalInformation += pRequest->Information;
    pRequest->Information = 0;

    //
    // Check if there are more transfers
    // in the sequence.
    //

    if (NT_SUCCESS(pRequest->Status))
    {
        pRequest->TransferIndex++;

        if (pRequest->TransferIndex < pRequest->TransferCount)
        {
            //
            // Configure the request for the next transfer.
            //

            pRequest->Status = PbcRequestConfigureForIndex(
                pRequest, 
                pRequest->TransferIndex);

            if (NT_SUCCESS(pRequest->Status))
            {
                //
                // Configure controller and kick-off read.
                // Request will be completed asynchronously.
                //

                PbcRequestDoTransfer(pDevice,pRequest);
                goto exit;
            }
        }
    }

    //
    // If not already cancelled, unmark request cancellable.
    //

    if (pRequest->Status != STATUS_CANCELLED)
    {
        NTSTATUS cancelStatus;
        cancelStatus = WdfRequestUnmarkCancelable(pRequest->SpbRequest);

        if (!NT_SUCCESS(cancelStatus))
        {
            //
            // WdfRequestUnmarkCancelable should only fail if the request
            // has already been or is about to be cancelled. If it does fail 
            // the request must NOT be completed - the cancel callback will do
            // this.
            //

            NT_ASSERTMSG("WdfRequestUnmarkCancelable should only fail if the request has already been or is about to be cancelled",
                cancelStatus == STATUS_CANCELLED);

            Trace(
                TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_TRANSFER,
                "Failed to unmark SPBREQUEST %p as cancelable - %!STATUS!",
                pRequest->SpbRequest,
                cancelStatus);

            goto exit;
        }
    }

    //
    // Done or error occurred. Set interrupt mask to 0.
    // Doing this keeps the DPC from re-enabling interrupts.
    //

    PbcDeviceSetInterruptMask(pDevice, 0);

    //
    // Clear the target's current request. This will prevent
    // the request context from being accessed once the request
    // is completed (and the context is invalid).
    //

    pDevice->pCurrentTarget->pCurrentRequest = NULL;

    //
    // Clear the controller's current target if any of
    //   1. request is type sequence
    //   2. request position is single 
    //      (did not come between lock/unlock)
    // Otherwise wait until unlock.
    //

    if ((pRequest->Type == SpbRequestTypeSequence) ||
        (pRequest->SequencePosition == SpbRequestSequencePositionSingle))
    {
        pDevice->pCurrentTarget = NULL;
    }

    //
    // Mark the IO complete. Request not
    // completed here.
    //

    pRequest->bIoComplete = TRUE;

exit:

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
ControllerEnableInterrupts(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask
    )
/*++
 
  Routine Description:

    This routine enables the hardware interrupts for the
    specificed mask.

  Arguments:

    pDevice - a pointer to the PBC device context
    InterruptMask - interrupt bits to enable

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice != NULL);

    Trace(
        TRACE_LEVEL_VERBOSE,
        TRACE_FLAG_TRANSFER,
        "Enable interrupts with mask 0x%lx (WDFDEVICE %p)",
        InterruptMask,
        pDevice->FxDevice);

    // TODO: Enable interrupts as requested.
	TwiEnableIrq(pDevice);

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_TRANSFER);
}

VOID
ControllerDisableInterrupts(
    _In_  PPBC_DEVICE   pDevice
    )
/*++
 
  Routine Description:

    This routine disables all controller interrupts.

  Arguments:

    pDevice - a pointer to the PBC device context

  Return Value:

    None.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    NT_ASSERT(pDevice != NULL);

    // TODO: Disable all interrupts.
	TwiDisableIrq(pDevice);

    UNREFERENCED_PARAMETER(pDevice);

    FuncExit(TRACE_FLAG_TRANSFER);
}

ULONG
ControllerGetInterruptStatus(
    _In_  PPBC_DEVICE   pDevice,
    _In_  ULONG         InterruptMask
    )
/*++
 
  Routine Description:

    This routine gets the interrupt status of the
    specificed interrupt bits.

  Arguments:

    pDevice - a pointer to the PBC device context
    InterruptMask - interrupt bits to check

  Return Value:

    A bitmap indicating which interrupts are set.

--*/
{
    FuncEntry(TRACE_FLAG_TRANSFER);

    ULONG interruptStatus = 0;

    NT_ASSERT(pDevice != NULL);

	interruptStatus = TwiQueryIrqStatus(pDevice);

    // TODO: Check if any of the interrupt mask
    //       bits have triggered an interrupt.
    
    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(InterruptMask);

    FuncExit(TRACE_FLAG_TRANSFER);

    return interruptStatus;
}

//VOID
//ControllerAcknowledgeInterrupts(
//    _In_  PPBC_DEVICE   pDevice,
//    _In_  ULONG         InterruptMask
//    )
///*++
// 
//  Routine Description:
//
//    This routine acknowledges the
//    specificed interrupt bits.
//
//  Arguments:
//
//    pDevice - a pointer to the PBC device context
//    InterruptMask - interrupt bits to acknowledge
//
//  Return Value:
//
//    None.
//
//--*/
//{
//    FuncEntry(TRACE_FLAG_TRANSFER);
//
//    NT_ASSERT(pDevice != NULL);
//
//    // TODO: Acknowledge requested interrupts.
//    
//    UNREFERENCED_PARAMETER(pDevice);
//    UNREFERENCED_PARAMETER(InterruptMask);
//
//    FuncExit(TRACE_FLAG_TRANSFER);
//}
