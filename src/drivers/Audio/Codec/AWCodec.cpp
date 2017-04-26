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

#include "AWCodec.h"
#include "Trace.h"
#include "AWCodec.tmh"

static volatile ULONG *pCCMURegMap = NULL;
static volatile ULONG *pCodecDigitalRegMap = NULL;
static volatile ULONG *pCodecAnalogRegMap = NULL;

//
// TODO: Define the register read/write operation as following.
//
UINT32 ReadCCMUReg(UINT32 offset)
{
	return READ_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCMURegMap + offset));
}

void WriteCCMUReg(UINT32 offset, UINT32 value)
{
	WRITE_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCMURegMap + offset), value);
}

//
// Read Codec analog register
//
UINT32 ReadAnalogReg(UINT32 Address)
{ 
	UINT32 Value;

	Value =READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value |= (0x1 << 28);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value &= ~(0x1 << 24);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);
	
	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value &= ~(0x1f << 16);
	Value |= (Address << 16);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value &= (0xff << 0);
	return Value;
}

void WriteAnalogReg(UINT32 Address, UINT32 Data)
{
	UINT32 Value;

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value |= (0x1 << 28);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value &= ~(0x1f << 16);
	Value |= (Address << 16);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value &= ~(0xff << 8);
	Value |= (Data << 8);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value |= (0x1 << 24);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

	Value = READ_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap));
	Value &= ~(0x1 << 24);
	WRITE_REGISTER_ULONG((volatile ULONG *)(pCodecAnalogRegMap), Value);

}

void CodecAnalogWriteControl(ULONG RegOffSet, ULONG Mask, ULONG Shift, ULONG Value)
{
	ULONG Old, New;
	Value = Value << Shift;
	Mask = Mask << Shift;
	Old = ReadAnalogReg(RegOffSet);
	New = (Old & ~Mask) | Value;
	WriteAnalogReg(RegOffSet, New);
}


UINT32 ReadDigitalReg(UINT32 offset)
{
	return READ_REGISTER_ULONG((volatile ULONG *)((ULONG)pCodecDigitalRegMap + offset));
}

void WriteDigitalReg(UINT32 offset, UINT32 value)
{
	WRITE_REGISTER_ULONG((volatile ULONG *)((ULONG)pCodecDigitalRegMap + offset), value);
}

void CodecDigitalWriteControl(ULONG RegOffSet, ULONG Mask, ULONG Shift, ULONG Value)
{
	ULONG Old, New;
	Value = Value << Shift;
	Mask = Mask << Shift;
	Old = ReadDigitalReg(RegOffSet);
	New = (Old & ~Mask) | Value;
	WriteDigitalReg(RegOffSet, New);
}

NTSTATUS AWCodecMapReg(void)
{

	NTSTATUS Status = STATUS_SUCCESS;
	FunctionEnter();

	int ret = 0;
	//
	// TODO: Map the I/O register base address as following
	//
	PHYSICAL_ADDRESS CodecDigitalRegMap = { 0 };
	PHYSICAL_ADDRESS CodecAnalogRegMap = { 0 };
	PHYSICAL_ADDRESS CodeCCMURegMap = { 0 };
	
	CodecDigitalRegMap.QuadPart = CODEC_DIGITAL_ADDRESS_BASE;
	CodecAnalogRegMap.QuadPart = CODEC_ANALOG_SHADOW_ADDRES_BASE;
	CodeCCMURegMap.QuadPart = CODEC_CCMU_ADDRESS_BASE;

	pCodecDigitalRegMap = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(CodecDigitalRegMap,CODEC_DIGITAL_ADDRESS_SIZE, MmNonCached));
	pCodecAnalogRegMap = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(CodecAnalogRegMap,CODEC_ANALOG_SHADOW_ADDRESS_SIZE,MmNonCached));
	pCCMURegMap = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(CodeCCMURegMap,CODEC_CCMU_ADDRESS_SIZE,MmNonCached));

	FunctionExit(ret);
	return Status;
}



//audio playback and capture initial
NTSTATUS AWCodecInitHw(void)
{
	NTSTATUS Status = STATUS_SUCCESS;
	FunctionEnter();

	//open the relevant clock in ccmu module
	//WriteCCMUReg(0x8, 0x90034e14); //22.5792M audio_pll_ctl reg
	WriteCCMUReg(0x8, 0x80035514); //24.576M audio_pll_ctl reg£¬48khz
	WriteCCMUReg(0x68, ReadCCMUReg(0x68) | 0x00000001);//open audio codec clock gate
	WriteCCMUReg(0x140, 0x80000000);//sclk=1 x clockout
	WriteCCMUReg(0x2d0, ReadCCMUReg(0x2d0) | 0x00000001); //audio codec bus soft de-assert

									 //init the digital register
	WriteDigitalReg(SUNXI_DA_CTL, 0x107);//bit1:rx enable,bit2,tx enable
	WriteDigitalReg(SUNXI_DA_FAT0, 0xc);
	WriteDigitalReg(SUNXI_DA_FCTL, 0x401f5);
	WriteDigitalReg(SUNXI_DA_INT, 0x88);
	WriteDigitalReg(SUNXI_DA_CLKD, 0x82);
	WriteDigitalReg(SUNXI_SYSCLK_CTL, 0xb38);
	WriteDigitalReg(SUNXI_MOD_CLK_ENA, 0x800c);
	WriteDigitalReg(SUNXI_MOD_RST_CTL, 0x800c);
	WriteDigitalReg(SUNXI_SYS_SR_CTRL, 0x8800);
	WriteDigitalReg(SUNXI_AIF1_CLK_CTRL, 0x8890);
	WriteDigitalReg(SUNXI_AIF1_ADCDAT_CTRL, 0xc000);
	WriteDigitalReg(SUNXI_AIF1_DACDAT_CTRL, 0xc000);
	WriteDigitalReg(SUNXI_AIF1_MXR_SRC, 0x2200);
	WriteDigitalReg(SUNXI_AIF1_VOL_CTRL1, 0xa0a0);
	WriteDigitalReg(SUNXI_AIF1_VOL_CTRL2, 0xa0a0);
	WriteDigitalReg(SUNXI_AIF1_VOL_CTRL3, 0xa0a0);
	WriteDigitalReg(SUNXI_AIF1_VOL_CTRL4, 0xa0a0);
	WriteDigitalReg(SUNXI_AIF1_MXR_GAIN, 0x0);
	WriteDigitalReg(SUNXI_ADC_DIG_CTRL, 0x8000);
	WriteDigitalReg(SUNXI_ADC_VOL_CTRL, 0xa0a0);
	WriteDigitalReg(SUNXI_DAC_DIG_CTRL, 0x8000);
	WriteDigitalReg(SUNXI_DAC_VOL_CTRL, 0xa0a0);
	WriteDigitalReg(SUNXI_DAC_MXR_SRC, 0x8800);
	WriteDigitalReg(SUNXI_DAC_MXR_GAIN, 0x0);

	//init the analog register
	WriteAnalogReg(HP_CTRL, 0x7b);//lowest 6bit controls the volume.
	WriteAnalogReg(OL_MIX_CTRL, 0x2);
	WriteAnalogReg(OR_MIX_CTRL, 0x2);
	WriteAnalogReg(MIC1_CTRL, 0x08);//only convert data as we have the mic array
	WriteAnalogReg(MIX_DAC_CTRL, 0xFC); //bit6 and bit7 to mute or dismute sound.
	WriteAnalogReg(L_ADCMIX_SRC, 0x40);
	WriteAnalogReg(R_ADCMIX_SRC, 0x40);
	WriteAnalogReg(HS_MBIAS_CTRL, 0xa1);
	WriteAnalogReg(ADC_CTRL, 0xc7);
	WriteAnalogReg(APT_REG, 0xd6);
	WriteAnalogReg(SPKOUT_CTRL0, 0xC0);
	WriteAnalogReg(SPKOUT_CTRL1, 0x1F);

#define AW1689_EVB
#ifdef AW1689_EVB
	WriteAnalogReg(MIC1_CTRL, 0x3c);
	WriteAnalogReg(ADC_CTRL, 0xc3);
#endif
	FunctionExit(Status);
	return Status;
}

NTSTATUS AWCodecStateControl(_In_ BOOL IsCapture,_In_ KSSTATE State) {

	
	NTSTATUS Status= STATUS_SUCCESS;
	ULONG CountReg = IsCapture ? SUNXI_DA_RXCNT : SUNXI_DA_TXCNT;
	ULONG FlushShift = IsCapture ? FRX : FTX;
	ULONG DrqShift = IsCapture ? RX_DRQ : TX_DRQ;
	switch (State) {
	case KSSTATE_STOP:
		//clear count reg
		WriteDigitalReg(CountReg, 0);
		//disable drq
		CodecDigitalWriteControl(SUNXI_DA_INT, 0x1, DrqShift, 0);
		//clear fifo
		CodecDigitalWriteControl(SUNXI_DA_FCTL, 0x1, FlushShift,1);
		break;
	case KSSTATE_ACQUIRE:
		break;
	case KSSTATE_PAUSE:
		//disable drq
		CodecDigitalWriteControl(SUNXI_DA_INT, 0x1, DrqShift, 0);
		//clear fifo
		CodecDigitalWriteControl(SUNXI_DA_FCTL, 0x1, FlushShift, 1);
		break;
	case KSSTATE_RUN:
		//clear count reg
		WriteDigitalReg(CountReg, 0);
		//clear fifo
		CodecDigitalWriteControl(SUNXI_DA_FCTL, 0x1, FlushShift, 1);
		//enable drq
		CodecDigitalWriteControl(SUNXI_DA_INT, 0x1, DrqShift, 1);
		break;
	 default:
		 Status =  STATUS_INVALID_PARAMETER;
	}
	return Status;
}
