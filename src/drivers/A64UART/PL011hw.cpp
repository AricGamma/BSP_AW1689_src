//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Module Name: 
//
//    PL011hw.cpp
//
// Abstract:
//
//    This module contains methods for accessing the ARM PL011
//    UART controller hardware.
//    This controller driver uses the Serial WDF class extension (SerCx2).
//
// Environment:
//
//    kernel-mode only
//
#include "precomp.h"
#pragma hdrstop

#define _PL011_HW_CPP_

// Logging header files
#include "PL011logging.h"
#include "PL011hw.tmh"

// Common driver header files
#include "PL011common.h"


static volatile ULONG *pCCURegMap = NULL;
static volatile ULONG *pGPIORegMap = NULL;


//TODO: Operate GPIO in here
UINT32 ReadGPIOReg(UINT32 offset)
{
	return READ_REGISTER_ULONG((volatile ULONG *)((ULONG)pGPIORegMap + offset));
}

void WriteGPIOReg(UINT32 offset, UINT32 value)
{
	WRITE_REGISTER_ULONG((volatile ULONG *)((ULONG)pGPIORegMap + offset), value);
}

void SetGPIOMode(UINT32 offset, UINT32 mask, UINT32 mode)
{
    UINT32 Value=0;
	Value = ReadGPIOReg(offset);
//	DbgPrintEx(1, 0, "SetGPIOMode, Read Value= 0x%x\r\n",Value);
	Value &= ~(0x7 << mask);
//	DbgPrintEx(1, 0, "SetGPIOMode, Value= 0x%x\r\n",Value);
	Value |= (mode<<mask);
//	DbgPrintEx(1, 0, "SetGPIOMode, Write Value= 0x%x\r\n",Value);
	WriteGPIOReg( offset, Value);
}

// Module specific header files


//
// Routine Description:
//
//  PL011HwInitController initializes the ARM PL011 controller, and  
//  sets it to a known state.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
// Return Value:
//
//  Controller initialization status.
//
_Use_decl_annotations_
NTSTATUS
PL011HwInitController(
	WDFDEVICE WdfDevice
)
{

    DbgPrintEx(1, 0, "PL011HwInitController is enter!\r\n");
	//PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
	
    //config UART  gpio mode.
	 PHYSICAL_ADDRESS GPIO_RegMap = { 0 };
	 GPIO_RegMap.QuadPart = GPIO_ADDRESS_BASE;
     pGPIORegMap = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(GPIO_RegMap,GPIO_ADDRESS_SIZE,MmNonCached));
	 
	 //uart2
	 SetGPIOMode(GPIO_PB_OFFSEET, 0, 0x2);////set GPIO_PB0 as UART2 TX MODE
	 SetGPIOMode(GPIO_PB_OFFSEET, 4, 0x2);////set GPIO_PB1 as UART2 RX MODE
	 SetGPIOMode(GPIO_PB_OFFSEET, 8, 0x2);////set GPIO_PB2 as UART2 RTS MODE
	 SetGPIOMode(GPIO_PB_OFFSEET, 12, 0x2);////set GPIO_PB3 as UART2 CTS MODE   

	//config UART CLOCK GATE
	{
	 UINT32 Value=0;
	 PHYSICAL_ADDRESS CodeCCMURegMap = { 0 };
	 CodeCCMURegMap.QuadPart = CCU_ADDRESS_BASE;
	 pCCURegMap = reinterpret_cast<volatile ULONG*>(MmMapIoSpace(CodeCCMURegMap,CCU_ADDRESS_SIZE,MmNonCached));
	 
	 //set clock souce APB2 as 24M
	// Value= READ_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCURegMap + 0X58));
	// DbgPrintEx(1, 0, "the CCUREG=0x%x\r\n",Value);
    // WRITE_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCURegMap + 0X58), (Value|0X1<<24));
	 
	 //set clock reset
	 Value = READ_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCURegMap + CCU_UART_RESET));
	 Value |= 0x1f0000; //bit 16~20   uart0~uart4
	 DbgPrintEx(1, 0, "just let me see, write CCU_UART_RESET Value= 0x%x\r\n",Value);
	 WRITE_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCURegMap + CCU_UART_RESET), Value);
	 
	 //set clock gate
	 Value = READ_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCURegMap + CCU_UART_CLOCK_GATE));
	 Value |= 0x1f0000; //bit 16~20   uart0~uart4
	 DbgPrintEx(1, 0, "just let me see, write CCU_UART_CLOCK_GATE Value= 0x%x\r\n",Value);
	 WRITE_REGISTER_ULONG((volatile ULONG *)((ULONG)pCCURegMap + CCU_UART_CLOCK_GATE), Value);
	 
	}
	
	//
	// config Modem control register      
	//
	PL011HwUartControl(
		WdfDevice,
		UARTCR_ALL, // All
		REG_UPDATE_MODE::BITMASK_CLEAR,
		nullptr
		); 

	//
	// Disable interrupts
	//
	PL011HwMaskInterrupts(
		WdfDevice,
		UART_INTERUPPTS_ALL, // All
		TRUE, // Mask,
		TRUE  // ISR safe
	);

	//
	// Clear pending interrupts, if any...
	//
	// PL011HwClearInterrupts(
		// devExtPtr, 
		// UART_INTERUPPTS_ALL
		// );

	//
	// Clear any errors, if any...
	//
	//PL011HwClearRxErros(devExtPtr);

	//
	// Configure FIFOs threshold
	//
	PL011HwSetFifoThreshold(
		WdfDevice,
		// UARTIFLS_RXIFLSEL::RXIFLSEL_1_4, // RX FIFO threshold >= 1/4 full
		// UARTIFLS_TXIFLSEL::TXIFLSEL_1_8  // TX FIFO threshold <= 1/8 full
		UARTIFLS_RXIFLSEL::RXIFLSEL_1,      //RX FIFO threshold =1/4
		UARTIFLS_TXIFLSEL::TXIFLSEL_1       //TX FIFO 2 char in the FIFO
       );
    //
    // Get supported baud rates if it has not 
    // already done.
    //
    PL011HwGetSupportedBaudRates(WdfDevice);
    //
    // Enable UART. 
    // RX and TX are enabled when the device is opened.
    //
    // PL011HwUartControl(
        // WdfDevice,
        // UARTCR_UARTEN,
        // REG_UPDATE_MODE::OVERWRITE,
        // nullptr
        // );

    PL011_LOG_INFORMATION("Controller initialization done!");

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  PL011HwStopController initializes stops the ARM PL011 controller, and  
//  resets it to a known state.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwStopController(
    WDFDEVICE WdfDevice
    )
{
	DbgPrintEx(1, 0, "PL011HwStopController is enter!\r\n");
    //
    // Disable the UART 
    //
    PL011HwUartControl(
        WdfDevice,
        UARTCR_ALL, // All
        REG_UPDATE_MODE::BITMASK_CLEAR,
        nullptr
        );

    //
    // Disable interrupts
    //
    PL011HwMaskInterrupts(
        WdfDevice,
        UART_INTERUPPTS_ALL, // All
        TRUE, // Mask,
        TRUE  // ISR safe
        );

    PL011_LOG_INFORMATION("Controller stop done!");
}


//
// Routine Description:
//
//  PL011HwGetSupportedBaudRates is called to find out what baud rates
//  are supported, given the UART clock.
//
//  The actual supported baud rate investigation is done once per the 
//  lifetime of the device, if the settable baud rates investigation 
//  was already done, routine does nothing.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwGetSupportedBaudRates(
    WDFDEVICE WdfDevice
    )
{
	DbgPrintEx(1, 0, "PL011HwGetSupportedBaudRates is enter!\r\n");
    //
    // Baud rate 
    //
    struct PL011_BAUD_RATE_DESCRIPTOR
    {
        // BAUD rate code SERIAL_BAUD_???
        ULONG   BaudCode;

        // Baud rate value in BPS
        ULONG   BaudBPS;

    } static baudValues[] = {
        { SERIAL_BAUD_9600,     9600  },
        { SERIAL_BAUD_14400,    14400 },
        { SERIAL_BAUD_19200,    19200 },
        { SERIAL_BAUD_38400,    38400 },
        { SERIAL_BAUD_57600,    57600 },
        { SERIAL_BAUD_115200,   115200},
    }; // baudValues

    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    if (devExtPtr->SettableBaud != 0) 
	{
        return;
    }
    devExtPtr->SettableBaud = SERIAL_BAUD_USER;

    for (ULONG descInx = 0; descInx < ULONG(ARRAYSIZE(baudValues)); ++descInx) 
	{

        PL011_BAUD_RATE_DESCRIPTOR* baudDescPtr = &baudValues[descInx];

        NTSTATUS status = PL011HwSetBaudRate(WdfDevice, baudDescPtr->BaudBPS);
        if (NT_SUCCESS(status)) 
		{

            devExtPtr->SettableBaud |= baudDescPtr->BaudCode;
        }

    } // for (baud rate descriptors)
}


//
// Routine Description:
//
//  PL011HwUartControl is called to modify the UART control register.
//  The routine either enable or disable controls, based on IsSetControl.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  UartControlMask - A combination of UARTCR_??? bits.
//
//  RegUpdateMode - How to update the control register.
//
//  OldUartControlPtr - Address of a caller ULONG var to receive the
//      old UART control, or nullptr if not needed.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwUartControl(
    WDFDEVICE WdfDevice,
    ULONG UartControlMask,
    REG_UPDATE_MODE RegUpdateMode,
    ULONG* OldUartControlPtr
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTCRPtr = PL011HwRegAddress(devExtPtr, UART_MCR);  //UARTCR

    //
    // Update UARTCR
    //

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);

    ULONG regUARTCR = PL011HwReadRegisterUlong(regUARTCRPtr);
	DbgPrintEx(1, 0, "just let me see, read UART_MCR Value= 0x%x\r\n",regUARTCR);

    if (OldUartControlPtr != nullptr) 
	{

        *OldUartControlPtr = regUARTCR;
    }

    switch (RegUpdateMode) 
	{
    case REG_UPDATE_MODE::BITMASK_SET:
        regUARTCR |= UartControlMask;
        break;

    case REG_UPDATE_MODE::BITMASK_CLEAR:
        regUARTCR &= ~UartControlMask;
        break;

    case REG_UPDATE_MODE::QUERY:
        goto done;

    case REG_UPDATE_MODE::OVERWRITE:
        regUARTCR = UartControlMask;
        break;

    default:
        PL011_LOG_ERROR(
            "Invalid register update mode %d", 
            ULONG(RegUpdateMode)
            );
        RegUpdateMode = REG_UPDATE_MODE::INVALID;
        goto done;
    }
	//regUARTCR &= UARTCR_ALL;

	DbgPrintEx(1, 0, "just let me see, write UART_MCR Value= 0x%x\r\n",regUARTCR);
    PL011HwWriteRegisterUlong(
        regUARTCRPtr,
        regUARTCR
        );

done:

    KeReleaseInStackQueuedSpinLock(&lockHandle);
    
    //
    // Log new UART control information
    //
    {
        static char* updateModeStrings[] = {
            "INVALID",
            "BITMASK_SET",
            "BITMASK_CLEAR",
            "QUERY",
            "OVERWRITE"
        };

        PL011_LOG_INFORMATION(
            "UART Control: update mode '%s', mask 0x%04X, actual 0x%04X",
            updateModeStrings[ULONG(RegUpdateMode)],
            USHORT(UartControlMask),
            USHORT(PL011HwReadRegisterUlong(regUARTCRPtr))
            );

    } // Log new UART control information
}


//
// Routine Description:
//
//  PL011HwSetFifoThreshold is called to set the UART FIFOs occupancy 
//  threshold for triggering RX/TX interrupts.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  RxInterruptTrigger - RX FIFO occupancy threshold interrupt trigger.
//
//  TxInterruptTrigger - TX FIFO occupancy threshold interrupt trigger.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwSetFifoThreshold(
    WDFDEVICE WdfDevice,
    UARTIFLS_RXIFLSEL RxInterruptTrigger,
    UARTIFLS_TXIFLSEL TxInterruptTrigger
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTIFLSPtr = PL011HwRegAddress(devExtPtr, UART_FCR);//UARTIFLS 

    PL011HwEnableFifos(WdfDevice, FALSE);

    //
    // Update the Interrupt FIFO level select register, UARTIFLS
    //
    {
        KLOCK_QUEUE_HANDLE lockHandle;
        KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);

        ULONG regUARTIFLS = PL011HwReadRegisterUlong(regUARTIFLSPtr);

        regUARTIFLS &= ~(UARTIFLS_TXIFLSEL_MASK | UARTIFLS_RXIFLSEL_MASK);
        regUARTIFLS |= (ULONG(RxInterruptTrigger) | ULONG(TxInterruptTrigger));

        PL011HwWriteRegisterUlong(regUARTIFLSPtr, regUARTIFLS);

        KeReleaseInStackQueuedSpinLock(&lockHandle);

    } // Update the Interrupt FIFO level select register, UARTIFLS

    PL011HwEnableFifos(WdfDevice, TRUE);

    PL011_LOG_INFORMATION(
        "UART FIFO triggers set to RX %lu, TX %lu, UARTIFLS 0x%04X",
        int(RxInterruptTrigger),
        int(TxInterruptTrigger),
        USHORT(PL011HwReadRegisterUlong(regUARTIFLSPtr))
        );
}


//
// Routine Description:
//
//  PL011HwMaskInterrupts is called to mask/unmask UART interrupts.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  InterruptBitMask - A combination of interrupt types to mask/unmask,
//      please refer to UARTIMSC_??? definitions for the various types
//      of interrupts.
//
//  IsMaskInterrupts - If to mask interrupts (TRUE), or unmask (FALSE).
//
//  IsIsrSafe - If to sync with ISR (TRUE), or not (FALSE). Usually
//      IsIsrSafe is set to FALSE when called from ISR, or from
//      framework Interrupt control callbacks.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwMaskInterrupts(
    WDFDEVICE WdfDevice,
    ULONG InterruptBitMask,
    BOOLEAN IsMaskInterrupts,
    BOOLEAN IsIsrSafe
    )
{
    PL011_ASSERT(
        ((IsIsrSafe == TRUE) && (KeGetCurrentIrql() <= DISPATCH_LEVEL)) ||
        ((IsIsrSafe == FALSE) && (KeGetCurrentIrql() > DISPATCH_LEVEL))
        );

    //
    // Update the Interrupt mask set/clear register, UARTIMSC
    //

    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTIMSCPtr = PL011HwRegAddress(devExtPtr, UART_IER); //UARTIMSC

    //
    // The following code should be synchronized with ISR code.
    //
    {
        if (IsIsrSafe)
		{
            WdfInterruptAcquireLock(devExtPtr->WdfUartInterrupt);
        }

        ULONG regUARTIMSC = PL011HwReadRegisterUlong(regUARTIMSCPtr);
        ULONG oldRegUARTIMSC = regUARTIMSC;

         if (IsMaskInterrupts) 
		 {
             regUARTIMSC &= ~InterruptBitMask;
         }
         else 
		 {
             regUARTIMSC |= InterruptBitMask;
         }
         regUARTIMSC &= UART_INTERUPPTS_ALL;

         PL011HwWriteRegisterUlong(regUARTIMSCPtr, regUARTIMSC);

        if (IsIsrSafe)
		{
            WdfInterruptReleaseLock(devExtPtr->WdfUartInterrupt);
        }

        PL011_LOG_TRACE(
            "%s events, old 0x%04X, mask 0x%04X, new 0x%04X",
            IsMaskInterrupts ? "Disabling" : "Enabling",
            USHORT(oldRegUARTIMSC),
            USHORT(InterruptBitMask),
            USHORT(PL011HwReadRegisterUlong(regUARTIMSCPtr))
            );

    } // ISR safe code
}


//
// Routine Description:
//
//  PL011HwSetBaudRate is called to configure the UART to a new baud rate.
//  The routine checks if the desired baud rate is supported:
//  - Desired baud rate is in range.
//  - Given the UART clock the error is within the allowed tolerance. 
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  BaudRateBPS - The desired baud rate in Bits Per Second (BPS)
//
// Return Value:
//
//  STATUS_SUCCESS, or STATUS_NOT_SUPPORTED if the desired 
//  baud rate is not supported.
//
_Use_decl_annotations_
NTSTATUS
PL011HwSetBaudRate(
    WDFDEVICE WdfDevice,
    ULONG BaudRateBPS
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);

    //
    // 1) Validate baud rate is in range
    //
    if ((BaudRateBPS < PL011_MIN_BAUD_RATE_BPS) ||
        (BaudRateBPS > devExtPtr->CurrentConfiguration.MaxBaudRateBPS)) {

        PL011_LOG_ERROR(
            "Baud rate out of range %lu, (%lu..%lu)",
            BaudRateBPS,
            PL011_MIN_BAUD_RATE_BPS,
            devExtPtr->CurrentConfiguration.MaxBaudRateBPS
            );
        return STATUS_NOT_SUPPORTED;
    }

    //
    // 2) Make sure requested baud rate is valid.
    //    UART clock should be at least 16 times the baud rate
    //
    ULONG uartClockHz = devExtPtr->CurrentConfiguration.UartClockHz;
	
	 DbgPrintEx(1, 0, "uartClockHz = 0x%x\r\n",uartClockHz);
    if (BaudRateBPS > (uartClockHz / 16)) 
	{
        PL011_LOG_ERROR(
            "Requested baud rate %lu should be less than UART clock (%lu) / 16",
            BaudRateBPS,
            uartClockHz
            );
        return STATUS_NOT_SUPPORTED;
    }

    //enter DLAB MODE frsit,when uart not busy.
	if( PL011HwIsBusy(devExtPtr)==FALSE)
	{ 
        DbgPrintEx(1, 0, "this is not busy, we can set band rate\r\n");
        PL011HwSetDivMode(WdfDevice, TRUE);
		
		//
        // 3) Calculate baud rate divisor:
        //
        //  BaudDivsior = UartCLockHz / (16 * BaudRateBPS)
        //  Where
        //    - regUART_DLL (8 bits) is the low 8bit of BaudDivisor.
        //    - regUART_DLH (8 bits) is the high 8bit of BaudDivisor.
        //
	     ULONG baudDivisor = uartClockHz  / (16 * BaudRateBPS);  //24m/(16*div)
		 DbgPrintEx(1, 0, "baudDivisor = 0x%x\r\n",baudDivisor);

    //
    // Calculate UART_DLL
    //
    ULONG regUART_DLL = baudDivisor & ULONG(0xFF);
	
	    //
        // Calculate UART_DLH
        //
         ULONG regUART_DLH = ((baudDivisor >> 8)& ULONG(0xFF));


    //
    // 4) Calculate the error, and make sure it is within the allowed range    256=2^8
    //
    ULONG actualBaudRateBPS = uartClockHz  / (16*(256 * regUART_DLH + regUART_DLL));
    int baudRateErrorPercent = int(actualBaudRateBPS - BaudRateBPS);
    if (baudRateErrorPercent < 0) 
	{
        baudRateErrorPercent *= -1;
    }
    baudRateErrorPercent = baudRateErrorPercent * 100 / BaudRateBPS;

    if (baudRateErrorPercent > PL011_MAX_BUAD_RATE_ERROR_PERCENT) 
	{

        PL011_LOG_ERROR(
            "Baud rate error out of range %lu%% > Max (%lu%%)",
            baudRateErrorPercent,
            PL011_MAX_BUAD_RATE_ERROR_PERCENT
            );
			
			PL011HwSetDivMode(WdfDevice, FALSE);
            return STATUS_NOT_SUPPORTED;
        }

    //
    // 5) Write to HW  
    //
    {
        KLOCK_QUEUE_HANDLE lockHandle;
        KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);

        // ULONG regUARTLCR_H = PL011HwReadRegisterUlong(
            // PL011HwRegAddress(devExtPtr, UARTLCR_H)
            // );
		
		//****************NEED AJUAGE BY GSY************************
        PL011HwWriteRegisterUlong(
            PL011HwRegAddress(devExtPtr, UART_DLL),
            regUART_DLL
            );
        PL011HwWriteRegisterUlong(
            PL011HwRegAddress(devExtPtr, UART_DLH),
            regUART_DLH
            );
		//**********************************************************

        // PL011HwWriteRegisterUlong(
            // PL011HwRegAddress(devExtPtr, UARTLCR_H),
            // regUARTLCR_H
            // );

        KeReleaseInStackQueuedSpinLock(&lockHandle);

    } // Write to HW  

    //
    // 6) Update current configuration
        //
        {
            KIRQL oldIrql = ExAcquireSpinLockExclusive(&devExtPtr->ConfigLock);

            devExtPtr->CurrentConfiguration.UartSerialBusDescriptor.BaudRate = BaudRateBPS;

            ExReleaseSpinLockExclusive(&devExtPtr->ConfigLock, oldIrql);

        } // Get current baud rate
		
		//leave DLAB MODE last,when uart not busy.
		PL011HwSetDivMode(WdfDevice, FALSE);
    }
	else
	{
		 DbgPrintEx(1, 0, "uart is in busy\r\n");
		 // return STATUS_NOT_SUPPORTED;
	}
	

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  PL011HwSetFlowControl is called to configure the UART flow control.
//  The routine verifies the desired flow control setup is supported,
//  and applies it to the UART.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  SerialFlowControlPtr - Address of a caller SERIAL_HANDFLOW var that
//      contains the desired flow control setup.
//
// Return Value:
//
//  STATUS_SUCCESS, or STATUS_NOT_SUPPORTED if the desired 
//  flow control setup is not supported by the SoC.
//
_Use_decl_annotations_
NTSTATUS
PL011HwSetFlowControl(
    WDFDEVICE WdfDevice,
    const SERIAL_HANDFLOW* SerialFlowControlPtr
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);

    //
    // Get current UART control
    //
     ULONG regUARTCR = 0;
    // PL011HwUartControl(
        // WdfDevice,
        // 0,
        // REG_UPDATE_MODE::QUERY,
        // &regUARTCR
        // );

    regUARTCR &= ~UART_FLOWCONTROL_En; //UARTCR_CTSEn;
    if ((SerialFlowControlPtr->ControlHandShake & SERIAL_CTS_HANDSHAKE) != 0) 
	{

        if ((devExtPtr->UartSupportedControlsMask & UART_FLOWCONTROL_En/*UARTCR_CTSEn*/) == 0) 
		{
            return STATUS_NOT_SUPPORTED;
        }
        regUARTCR |= UART_FLOWCONTROL_En;//UARTCR_CTSEn;

    } // SERIAL_CTS_HANDSHAKE

    // regUARTCR &= ~UARTCR_RTSEn;
    // if ((SerialFlowControlPtr->FlowReplace & SERIAL_RTS_HANDSHAKE) != 0) {

        // if ((devExtPtr->UartSupportedControlsMask & UARTCR_RTSEn) == 0) {

            // return STATUS_NOT_SUPPORTED;
        // }
        // regUARTCR |= UARTCR_RTSEn;

    // } // SERIAL_RTS_HANDSHAKE

    regUARTCR &= ~UART_RTS;//~UARTCR_RTS;
    if ((SerialFlowControlPtr->FlowReplace & SERIAL_RTS_CONTROL) != 0) {

        if ((devExtPtr->UartSupportedControlsMask & UART_RTS) == 0) {

            return STATUS_NOT_SUPPORTED;
        }
        regUARTCR |= UART_RTS;//UARTCR_RTS;

    } // SERIAL_RTS_CONTROL

    regUARTCR &= ~UART_DTR;//~UARTCR_DTR;
    if ((SerialFlowControlPtr->ControlHandShake & SERIAL_DTR_CONTROL) != 0) {

        if ((devExtPtr->UartSupportedControlsMask & UART_DTR) == 0) {

            return STATUS_NOT_SUPPORTED;
        }
        regUARTCR |= UART_DTR;//UARTCR_DTR;

    } // SERIAL_DTR_CONTROL

    //
    // Now set the new UART control
    //
    // PL011HwUartControl(
        // WdfDevice,
        // regUARTCR,
        // REG_UPDATE_MODE::OVERWRITE,
        // nullptr
        // );

    //
    // Save new flow control setup
    //
    {
        KIRQL oldIrql = ExAcquireSpinLockExclusive(&devExtPtr->ConfigLock);

        devExtPtr->CurrentConfiguration.FlowControlSetup =
            *SerialFlowControlPtr;

        ExReleaseSpinLockExclusive(&devExtPtr->ConfigLock, oldIrql);

    } // Get current line control setup

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  PL011HwSetLineControl is called to configure the UART line control.
//  The routine maps the UART generic settings into PL011 registers
//  and applies the new setup.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  SerialLineControlPtr - Address of a caller SERIAL_LINE_CONTROL var that
//      contains the desired line control setup.
//
// Return Value:
//
//  STATUS_SUCCESS, STATUS_NOT_SUPPORTED if the desired line control setup 
//  is not supported, STATUS_INVALID_PARAMETER.
//
_Use_decl_annotations_
NTSTATUS
PL011HwSetLineControl(
    WDFDEVICE WdfDevice,
    const SERIAL_LINE_CONTROL* SerialLineControlPtr
    )
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTLCR_HPtr = PL011HwRegAddress(devExtPtr,UART_LCR );//UARTLCR_H

	DbgPrintEx(1, 0, "PL011HwSetLineControl is enter!\r\n");
    //
    // Disable the UART 
    //
    //ULONG regUARTCR;
    //PL011HwUartControl(
    //    WdfDevice,
    //    UARTCR_UARTEN,
    //    REG_UPDATE_MODE::BITMASK_CLEAR,
    //    &regUARTCR
    //    );

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);
   
    ULONG regUARTLCR_H = PL011HwReadRegisterUlong(regUARTLCR_HPtr);

    //
    // Set word length
    //
    regUARTLCR_H &= ~UARTLCR_WLEN_MASK;
	

    switch (SerialLineControlPtr->WordLength) 
	{

    case 5:
        regUARTLCR_H |= ULONG(UARTLCR_WLEN::WLEN_5BITS);
        break;

    case 6:
        regUARTLCR_H |= ULONG(UARTLCR_WLEN::WLEN_6BITS);
        break;

    case 7:
        regUARTLCR_H |= ULONG(UARTLCR_WLEN::WLEN_7BITS);
        break;

    case 8:
        regUARTLCR_H |= ULONG(UARTLCR_WLEN::WLEN_8BITS);
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        goto done;

    } // switch (WordLength)

    //
    // Set stop bits
    //
    int stopBits;
    switch (SerialLineControlPtr->StopBits) 
	{

    case STOP_BIT_1:
        regUARTLCR_H &= ~STOP_BIT;
        stopBits = 1;
        break;

    case STOP_BITS_2:
        regUARTLCR_H |= STOP_BIT;
        stopBits = 2;
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        goto done;

    } // switch (StopBits)

    //
    // Parity
    //
    const char* partityStrSz;
    switch (SerialLineControlPtr->Parity) {

    case NO_PARITY:
        regUARTLCR_H &= ~PARITY_ENABLE;
        partityStrSz = "NONE";
        break;

    case ODD_PARITY:
        regUARTLCR_H |= PARITY_ENABLE;
		regUARTLCR_H &= ~UART_EVEN_PARITY;
		// regUARTLCR_H &= ~UARTLCR_SPS;
        partityStrSz = "ODD";
        break;

    case EVEN_PARITY:
        regUARTLCR_H |= PARITY_ENABLE;
		regUARTLCR_H |= UART_EVEN_PARITY;
        //regUARTLCR_H &= ~UARTLCR_SPS;
        partityStrSz = "EVEN";
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        goto done;

    } // switch (Parity)

    //
    // Apply new line control setup
    //
    PL011HwWriteRegisterUlong(regUARTLCR_HPtr, regUARTLCR_H);

    //
    // Save current line control setup
    //
    {
        KIRQL oldIrql = ExAcquireSpinLockExclusive(&devExtPtr->ConfigLock);

        devExtPtr->CurrentConfiguration.LineControlSetup =
            *SerialLineControlPtr;

        ExReleaseSpinLockExclusive(&devExtPtr->ConfigLock, oldIrql);

    } // Get current line control setup

    status = STATUS_SUCCESS;


done:

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    // PL011HwUartControl(
        // WdfDevice,
        // regUARTCR, // Restore
        // REG_UPDATE_MODE::OVERWRITE,
        // nullptr
        // );

    return status;
}


//
// Routine Description:
//
//  PL011HwEnableFifos is called to enable/disable RX/TX FIFOs.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  IsEnable - If to enable (TRUE), or disable (FALSE) FIFOs
//
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwEnableFifos(
    WDFDEVICE WdfDevice,
    BOOLEAN IsEnable
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTLCR_HPtr = PL011HwRegAddress(devExtPtr, UART_FCR);//PL011HwRegAddress(devExtPtr, UARTLCR_H);

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);

    ULONG regUARTLCR_H = PL011HwReadRegisterUlong(regUARTLCR_HPtr);

    if (IsEnable) 
	{

        regUARTLCR_H |= UART_FIFOE; //UARTLCR_FEN;

    } 
	else 
	{

        regUARTLCR_H &= ~UART_FIFOE; //UARTLCR_FEN;
    }

    PL011HwWriteRegisterUlong(regUARTLCR_HPtr, regUARTLCR_H);

    KeReleaseInStackQueuedSpinLock(&lockHandle);
}


//
// Routine Description:
//
//  PL011HwSetModemControl is called to assert modem control signals.
//  The routine verifies that modem control is supported by the SoC
//  UART and applies the setting to the hardware.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  ModemControl - Please refer to  SERIAL_MCR_??? for bit definitions.
//
// Return Value:
//  
//  STATUS_SUCCESS, or STATUS_NOT_SUPPORTED if current SoC does not expose
//      modem control lines.
//
_Use_decl_annotations_
NTSTATUS
PL011HwSetModemControl(
    WDFDEVICE WdfDevice,
    UCHAR ModemControl
    )
{
    const PL011_DEVICE_EXTENSION* devExtPtr = 
        PL011DeviceGetExtension(WdfDevice);

    //
    // Get current UART control
    //
    ULONG regUARTCR = 0;
    // PL011HwUartControl(
        // WdfDevice,
        // 0,
        // REG_UPDATE_MODE::QUERY,
        // &regUARTCR
        // );

    regUARTCR &= ~UART_FLOWCONTROL_En;//~UARTCR_CTSEn;
    if ((ModemControl & SERIAL_MCR_CTS_EN) != 0) 
	{

        if ((devExtPtr->UartSupportedControlsMask & UART_FLOWCONTROL_En) == 0) //UARTCR_CTSEn
		{

            return STATUS_NOT_SUPPORTED;
        }
        regUARTCR |= UART_FLOWCONTROL_En;//UARTCR_CTSEn;

    } // SERIAL_CTS_HANDSHAKE

    // regUARTCR &= ~UARTCR_RTSEn;
    // if ((ModemControl & SERIAL_MCR_RTS_EN) != 0) {

        // if ((devExtPtr->UartSupportedControlsMask & UARTCR_RTSEn) == 0) {

            // return STATUS_NOT_SUPPORTED;
        // }
        // regUARTCR |= UARTCR_RTSEn;

    // } // SERIAL_RTS_HANDSHAKE

    regUARTCR &= ~UART_RTS;//~UARTCR_RTS;
    if ((ModemControl & SERIAL_MCR_RTS) != 0) 
	{

        if ((devExtPtr->UartSupportedControlsMask & UART_RTS/*UARTCR_RTS*/) == 0) 
		{

            return STATUS_NOT_SUPPORTED;
        }
        regUARTCR |= UART_RTS;//UARTCR_RTS;

    } // SERIAL_RTS_CONTROL

    regUARTCR &= ~UART_DTR;//~UARTCR_DTR;
    if ((ModemControl & SERIAL_MCR_DTR) != 0) {

        if ((devExtPtr->UartSupportedControlsMask & UART_DTR/*UARTCR_DTR*/) == 0) {

            return STATUS_NOT_SUPPORTED;
        }
        regUARTCR |= UART_DTR;//UARTCR_DTR;

    } // SERIAL_DTR_CONTROL

    regUARTCR &= ~UART_LOOP;
    if ((ModemControl & SERIAL_MCR_LOOP) != 0) {

        regUARTCR |= UART_LOOP;

    } // SERIAL_MCR_LOOP

	regUARTCR |= (1 << 0) | (1 << 1) | (0 << 3) | (0 << 4) | (0 << 5);

    PL011HwUartControl(
        WdfDevice,
        regUARTCR,
        REG_UPDATE_MODE::OVERWRITE,
        nullptr
        );

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  PL011HwGetModemControl is called to query modem control signals state.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  ModemControlPtr - Address of a caller UCGAR to receive the current 
//      modem control.
//      Please refer to  SERIAL_MCR_??? for bit definitions.
//
// Return Value:
//  
//  STATUS_SUCCESS.
//
_Use_decl_annotations_
NTSTATUS
PL011HwGetModemControl(
    WDFDEVICE WdfDevice,
    UCHAR* ModemControlPtr
    )
{
    *ModemControlPtr = 0;

    //
    // Get current UART control
    //
    UCHAR modemControl = 0;
    ULONG regUARTCR;
     PL011HwUartControl(
         WdfDevice,
         0,
         REG_UPDATE_MODE::QUERY,
         &regUARTCR
         );

    if ((regUARTCR & UART_FLOWCONTROL_En) != 0) {

        modemControl |= SERIAL_MCR_CTS_EN;
    }

    if ((regUARTCR & UART_FLOWCONTROL_En) != 0) {

        modemControl |= SERIAL_MCR_RTS_EN;
    }

    if ((regUARTCR & UART_RTS) != 0) {

        modemControl |= SERIAL_MCR_RTS;
    }

    if ((regUARTCR & UART_DTR) != 0) {

        modemControl |= SERIAL_MCR_DTR;
    }

    if ((regUARTCR & UART_LOOP) != 0) {

        modemControl |= SERIAL_MCR_LOOP;
    }

    //if ((regUARTCR & UARTCR_Out1) != 0) {

    //    modemControl |= SERIAL_MCR_OUT1;
    //}

    //if ((regUARTCR & UARTCR_Out2) != 0) {

    //    modemControl |= SERIAL_MCR_OUT2;
    //}

    *ModemControlPtr = modemControl;

    return STATUS_SUCCESS;
}


//
// Routine Description:
//
//  PL011HwSetBreak is called to send/clear break.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
//  IsBreakON - If to send (TRUE), or clear (FALSE), break.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwSetBreak(
    WDFDEVICE WdfDevice,
    BOOLEAN IsBreakON
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTLCR_HPtr = PL011HwRegAddress(devExtPtr, UART_LCR);

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);

    ULONG regUARTLCR_H = PL011HwReadRegisterUlong(regUARTLCR_HPtr);

    if (IsBreakON) {

        regUARTLCR_H |= UART_BC;

    } else {

        regUARTLCR_H &= ~UART_BC;
    }

    PL011HwWriteRegisterUlong(regUARTLCR_HPtr, regUARTLCR_H);

    KeReleaseInStackQueuedSpinLock(&lockHandle);
}


//
// Routine Description:
//
//  PL011HwRegsDump is called to dump important PL011 registers.
//
// Arguments:
//
//  WdfDevice - The WdfDevice object the represent the PL011 this instance of
//      the PL011 controller.
//
// Return Value:
//
_Use_decl_annotations_
VOID
PL011HwRegsDump(
    WDFDEVICE WdfDevice
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
	
	PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UART_LCR));

    // PL011_LOG_TRACE(
        // "UARTCR %04X, "
        // "UARTIBRD %04X, "
        // "UARTFBRD %04X, "
        // "UARTLCR_H %04X, "
        // "UARTIMSC %04X, "
        // "UARTIFLS %04X, "
        // "UARTRIS %04X, "
        // "UARTFR %04X",
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTCR)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTIBRD)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTFBRD)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTLCR_H)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTIMSC)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTIFLS)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTRIS)),
        // PL011HwReadRegisterUlong(PL011HwRegAddress(devExtPtr, UARTFR))
        // );
}

_Use_decl_annotations_
VOID
PL011HwSetDivMode(
    WDFDEVICE WdfDevice,
    BOOLEAN IsON
    )
{
    PL011_DEVICE_EXTENSION* devExtPtr = PL011DeviceGetExtension(WdfDevice);
    volatile ULONG* regUARTLCR_HPtr = PL011HwRegAddress(devExtPtr, UART_LCR);

    KLOCK_QUEUE_HANDLE lockHandle;
    KeAcquireInStackQueuedSpinLock(&devExtPtr->RegsLock, &lockHandle);

    ULONG regUARTLCR_H = PL011HwReadRegisterUlong(regUARTLCR_HPtr);

    if (IsON) {

        regUARTLCR_H |= UART_DLAB;

    } else {

        regUARTLCR_H &= ~UART_DLAB;
    }

    PL011HwWriteRegisterUlong(regUARTLCR_HPtr, regUARTLCR_H);

    KeReleaseInStackQueuedSpinLock(&lockHandle);
}

#undef _PL011_HW_CPP_