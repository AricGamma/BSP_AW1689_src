#pragma once
#include "Driver.h"
#include "Device.h"


//CIR reg configuration
#define CIR_ROI_EN
#define CIR_RPEI_EN
#define CIR_RAI_EN
//#define CIR_DRQ_EN
#define CIR_RX_FIFO_TRGGER_LEVEL 0x14


//CIR MODE GPIO Register
#define CIR_MODE_GPIO_REG_BASE 0x01F02C00		//Register base
#define CIR_MODE_GPIO_REG_OFFSET 0x4			//GPIO ctrl register 1 offset
#define CIR_MODE_GPIO_REG_MASK 0xc				//PL11 ctrl mask
#define CIR_MODE_GPIO_REG_LENGTH 0x21C

#define CIR_MODE_GPIO_INPUT 0x00
#define CIR_MODE_GPIO_OUTPUT 0x01
#define CIR_MODE_GPIO_CIR_RX 0x02
#define CIR_MODE_GPIO_EINT 0x06
#define CIR_MODE_GPIO_DISABLE 0x07


#define PRCM_REG_BASE 0x01F01400
#define PRCM_REG_LENGTH 0x1D4

#define CIR_GATING_REG_OFFSET 0x28
#define CIR_GATING_REG_MASK 0x1

#define CIR_RST_REG_OFFSET 0xB0
#define CIR_RST_REG_MASK 0x1

#define CIR_GATING_MASK 0x0
#define CIR_GATING_PASS 0x1

#define CIR_RST_ASSERT 0x0
#define CIR_RST_DE_ASSERT 0x1

#define CIR_CLK_SRC_REG_OFFSET 0x54
#define CIR_CLK_DIV_RATIO_M_MASK 0x0
#define CIR_CLK_DIV_RATIO_N_MASK 0x16
#define CIR_CLK_SRC_SEL_MASK 0x24
#define CIR_CLK_SCLK_GATING_MASK 0x31

#define CIR_CLK_SRC_32K 0x0
#define CIR_CLK_SRC_OSC24M 0x1


#define CIR_INT_REG_OFFSET 0x2C

#define CIR_ROI_EN_MASK 0x0
#define CIR_ROI_DISABLE 0x0
#define CIR_ROI_ENABLE 0x1

#define CIR_RPEI_EN_MASK 0x1
#define CIR_RPEI_DISABLE 0x0
#define CIR_RPEI_ENABLE 0x1

#define CIR_RAI_EN_MASK 0x4
#define CIR_RAI_DISABLE 0x0
#define CIR_RAI_ENABLE 0x1

#define CIR_DRQ_EN_MASK 0x5
#define CIR_DRQ_DISABLE 0x0
#define CIR_DRQ_ENABLE 0x1

#define CIR_RAL_MASK 0x8



#define CIR_RX_CTRL_REG_OFFSET 0x10
#define CIR_RX_CTRL_RPPI 0x1
#define CIR_RX_CTRL_NOT_RPPI 0x0


#define CIR_RXFIFO_REG_OFFSET 0x20
#define CIR_RXFIFO_LENGTH 0x1


#define CIR_RX_STATE_REG_OFFSET 0x30

#define CIR_RX_STATE_ROI_MASK 0x0
#define CIR_RX_STATE_ROI 0x1
#define CIR_RX_STATE_NOT_ROI 0x0

#define CIR_RX_STATE_RPE_MASK 0x1
#define CIR_RX_STATE_RPE 0x1
#define CIR_RX_STATE_NOT_RPE 0x0

#define CIR_RX_STATE_RA_MASK 0x4
#define CIR_RX_STATE_RA 0x1
#define CIR_RX_STATE_NOT_RA 0x0

#define CIR_RX_STATE_IDLE_MASK 0x7
#define CIR_RX_STATE_IDLE 0x0
#define CIR_RX_STATE_BUSY 0x1

#define CIR_RX_STATE_RAC_MASK 0x8
#define CIR_RX_STATE_RAC_LENGTH 0x7





NTSTATUS CIRInitialize(_In_ WDFDEVICE Device);

NTSTATUS GpioModeSetCirRx();
NTSTATUS GatingCir();
NTSTATUS ResetCir();
VOID ConfigClkSource();
VOID CIRInterruptConfig(_In_ WDFDEVICE Device);

ULONG ReadReg(_In_ volatile ULONG * RegBase, _In_ UINT32 Offset);

VOID WriteReg(_In_ volatile ULONG * RegBase, _In_ UINT32 Offset, _In_ UINT32 Mask, _In_ UINT8 Value);

