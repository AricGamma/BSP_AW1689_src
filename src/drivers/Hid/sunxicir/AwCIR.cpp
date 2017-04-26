#include "AwCIR.h"

#include "Trace.h"
#include "AwCIR.tmh"

static volatile ULONG * pCirModeGpioReg = NULL;
static volatile ULONG * pPRCMRegMap = NULL;

NTSTATUS CIRInitialize(_In_ WDFDEVICE Device)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevContext;

	FunctionEnter();

	pDevContext = DeviceGetContext(Device);

	//Map cir mode gpio reg
	PHYSICAL_ADDRESS cirModeGpioRegAddr = { 0 };
	cirModeGpioRegAddr.LowPart = CIR_MODE_GPIO_REG_BASE;

	pCirModeGpioReg = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(cirModeGpioRegAddr, CIR_MODE_GPIO_REG_LENGTH, MmNonCached));

	//Map PRCM reg
	PHYSICAL_ADDRESS PRCMRegAddr = { 0 };
	PRCMRegAddr.LowPart = PRCM_REG_BASE;
	pPRCMRegMap = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(PRCMRegAddr, PRCM_REG_LENGTH, MmNonCached));

	//Set GPIO PL11 to CIR_RX mode
	status = GpioModeSetCirRx();
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Error: Set Gpio mode to CIR RX failed");
		goto Exit;
	}


	//Gating CIR_RX
	status = GatingCir();
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Error: Gating CIR_RX failed");
		goto Exit;
	}


	//Reset CIR_RX
	status = ResetCir();
	if (!NT_SUCCESS(status))
	{
		DbgPrint_E("Error: Reset CIR_RX failed");
		goto Exit;
	}

	//config clock src
	ConfigClkSource();


	//write reg to enable CIR
	WRITE_REGISTER_ULONG(pDevContext->RegisterBase, 0x33);

	//config interrupt reg
	CIRInterruptConfig(Device);

	//config CIR_RCR
	WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)pDevContext->RegisterBase + 0x34), 0x00800543);
	//WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)pDevContext->RegisterBase + 0x10), 0x0);

	

Exit:

	return status;
}


NTSTATUS GpioModeSetCirRx() 
{
	NTSTATUS status = STATUS_SUCCESS;
	if (pCirModeGpioReg != NULL)
		WriteReg(pCirModeGpioReg, CIR_MODE_GPIO_REG_OFFSET, CIR_MODE_GPIO_REG_MASK, CIR_MODE_GPIO_CIR_RX);
	else
		status = STATUS_UNSUCCESSFUL;

	return status;
}

NTSTATUS GatingCir()
{
	NTSTATUS status = STATUS_SUCCESS;
	if (pPRCMRegMap != NULL)
		WriteReg(pPRCMRegMap, CIR_GATING_REG_OFFSET, CIR_GATING_REG_MASK, CIR_GATING_PASS);
	else
		status = STATUS_UNSUCCESSFUL;

	return status;
}

NTSTATUS ResetCir()
{
	NTSTATUS status = STATUS_SUCCESS;
	if (pPRCMRegMap != NULL)
	{
		WriteReg(pPRCMRegMap, CIR_RST_REG_OFFSET, CIR_RST_REG_MASK, CIR_RST_DE_ASSERT);
	}
	else
		status = STATUS_UNSUCCESSFUL;
	
	return status;
}

VOID ConfigClkSource()
{
	//WriteReg(pPRCMRegMap, CIR_CLK_SRC_REG_OFFSET, CIR_CLK_SCLK_GATING_MASK, 0x1);
	//WriteReg(pPRCMRegMap, CIR_CLK_SRC_REG_OFFSET, CIR_CLK_SRC_SEL_MASK, CIR_CLK_SRC_OSC24M);
	WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)pPRCMRegMap + CIR_CLK_SRC_REG_OFFSET), 0x81000000);
}

VOID CIRInterruptConfig(WDFDEVICE Device)
{
	PDEVICE_CONTEXT pDevContext = NULL;

	pDevContext = DeviceGetContext(Device);

#ifdef CIR_ROI_EN

	WriteReg(pDevContext->RegisterBase, CIR_INT_REG_OFFSET, CIR_ROI_EN_MASK, CIR_ROI_ENABLE);

#endif

#ifdef CIR_RPEI_EN

	WriteReg(pDevContext->RegisterBase, CIR_INT_REG_OFFSET, CIR_RPEI_EN_MASK, CIR_RPEI_ENABLE);

#endif

#ifdef CIR_RAI_EN

	WriteReg(pDevContext->RegisterBase, CIR_INT_REG_OFFSET, CIR_RAI_EN_MASK, CIR_RAI_ENABLE);

#endif

#ifdef CIR_DRQ_EN
	WriteReg(pDevContext->RegisterBase, CIR_INT_REG_OFFSET, CIR_DRQ_EN_MASK, CIR_DRQ_ENABLE);
#endif

	WriteReg(pDevContext->RegisterBase, CIR_INT_REG_OFFSET, CIR_RAL_MASK, CIR_RX_FIFO_TRGGER_LEVEL - 1);
}


VOID WriteReg(volatile ULONG * RegBase, UINT32 offset, UINT32 mask, UINT8 value)
{
	ULONG preValue;
	preValue = ReadReg(RegBase, offset);

	ULONG newValue = (~(0x7 << mask) & preValue) | (value << mask);
	WRITE_REGISTER_ULONG((volatile ULONG*)((ULONG)RegBase + offset), newValue);
}


ULONG ReadReg(volatile ULONG *RegBase, UINT32 offset)
{
	return READ_REGISTER_ULONG((volatile ULONG *)((ULONG)RegBase + offset));
}
