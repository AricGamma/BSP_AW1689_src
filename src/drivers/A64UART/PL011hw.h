//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Module Name: 
//
//    PL011hw.h
//
// Abstract:
//
//    This module contains common enum, types, and methods definitions
//    for accessing the ARM PL011 UART controller hardware.
//    This controller driver uses the Serial WDF class extension (SerCx2).
//
// Environment:
//
//    kernel-mode only
//

#ifndef _PL011_HW_H_
#define _PL011_HW_H_

WDF_EXTERN_C_START


//
// PL011_REG_FILE:
//  PL011 registers offsets
//
// typedef enum _PL011_REG_FILE {
    // UARTDR          = 0x0000,
    // UARTRSR_ECR     = 0x0004,
    ////Reserved 0x0008-0x0014
    // UARTFR          = 0x0018,
    ////Reserved 0x001C
    // UARTILPR        = 0x0020,   
    // UARTIBRD        = 0x0024,
    // UARTFBRD        = 0x0028,
    // UARTLCR_H       = 0x002C,
    // UARTCR          = 0x0030,
    // UARTIFLS        = 0x0034,
    // UARTIMSC        = 0x0038,
    // UARTRIS         = 0x003C,
    // UARTMIS         = 0x0040,
    // UARTICR         = 0x0044,
    // UARTDMACR       = 0x0048,
    ////Reserved 0x004C-0x007C
    // UARTITCR        = 0x0080,
    // UARTITIP        = 0x0084,
    // UARTITOP        = 0x0084,
    // UARTTDR         = 0x008C,
    ////Reserved 0x0090-0x0FDC
    // UARTPeriphID0   = 0x0FE0,
    // UARTPeriphID1   = 0x0FE4,
    // UARTPeriphID2   = 0x0FE8,
    // UARTPeriphID3   = 0x0FEC,
    // UARTPCellID0    = 0x0FF0,
    // UARTPCellID1    = 0x0FF4,
    // UARTPCellID2    = 0x0FF8,
    // UARTPCellID3    = 0x0FEC,

    // REG_FILE_SIZE   = 0x1000
// } PL011_REG_FILE;


//
// Stride between to PL011 consecutive registers
//
#define PL1011_REG_STRIDE 4


//
// PL011 UART Clock
// Based on Raspberry Pi2 typical settings.
// (Can be overwritten by registry settings)
//
#define ONE_MHZ                     ULONG(1000000)
#define PL011_DEAFULT_UART_CLOCK    ULONG(24 * ONE_MHZ)//ULONG(16 * ONE_MHZ)


//
//  RX/TX FIFOs depth
//
#define PL011_FIFO_DEPTH    64



#if 0   // disalbe by gsy 
//
// PL011 Receive status register/error clear register (UARTRSR_ECR) 
// fields definition
//
#define UARTRSR_ECR_FE  (ULONG(1 << 0))     // Framing error
#define UARTRSR_ECR_PE  (ULONG(1 << 1))     // Parity error
#define UARTRSR_ECR_BE  (ULONG(1 << 2))     // Break error
#define UARTRSR_ECR_OE  (ULONG(1 << 3))     // Overrun error

//
// PL011 Flag register (UARTFR) fields definition
//
#define UARTFR_CTS  (ULONG(1 << 0))     // Clear to send
#define UARTFR_DSR  (ULONG(1 << 1))     // Data set ready
#define UARTFR_DCD  (ULONG(1 << 2))     // Data carrier detect
#define UARTFR_BUSY (ULONG(1 << 3))     // Busy (TX)
#define UARTFR_RXFE (ULONG(1 << 4))     // RX FIFO empty
#define UARTFR_TXFF (ULONG(1 << 5))     // TX FIFO full
#define UARTFR_RXFF (ULONG(1 << 6))     // RX FIFO full
#define UARTFR_TXFE (ULONG(1 << 7))     // TX FIFO empty
#define UARTFR_RI   (ULONG(1 << 8))     // Ring indicator

//
// PL011 Integer baud rate register (UARTIBRD)
// 16 bits
//
#define UARTIBRD_MASK   (ULONG(0x0000FFFF))

//
// PL011 Fractional baud rate register (UARTFBRD)
// 5 bits
//
#define UARTFBRD_MASK   (ULONG(0x0000001F))

//
// PL011 Line control register (UARTLCR_H) fields definition
//
#define UARTLCR_BRK         (ULONG(1 << 0)) // Send break
#define UARTLCR_PEN         (ULONG(1 << 1)) // Parity enable
#define UARTLCR_EPS         (ULONG(1 << 2)) // Even parity select
#define UARTLCR_STP2        (ULONG(1 << 3)) // Two stop bits select
#define UARTLCR_FEN         (ULONG(1 << 4)) // Enable FIFOs
#define UARTLCR_WLEN_MASK   (ULONG(3 << 5)) // Word length

    typedef enum _UARTLCR_WLEN : ULONG {
       WLEN_8BITS = (ULONG(3 << 5)), // 8 bit word
       WLEN_7BITS = (ULONG(2 << 5)), // 7 bit word
       WLEN_6BITS = (ULONG(1 << 5)), // 6 bit word
       WLEN_5BITS = (ULONG(0 << 5)), // 5 bit word
    } UARTLCR_WLEN;
#define UARTLCR_SPS         (ULONG(1 << 7)) // Stick parity select

//
// PL011 control register (UARTCR) fields definition
//
#define UARTCR_UARTEN   (ULONG(1 << 0))     // UART enable
#define UARTCR_SIREN    (ULONG(1 << 1))     // SIR enable
#define UARTCR_SIRLP    (ULONG(1 << 2))     // IrDA SIR low power mode
#define UARTCR_LBE      (ULONG(1 << 7))     // Loop back enable
#define UARTCR_TXE      (ULONG(1 << 8))     // Transmit enable
#define UARTCR_RXE      (ULONG(1 << 9))     // Receive enable
#define UARTCR_DTR      (ULONG(1 << 10))    // Data transmit ready
#define UARTCR_RTS      (ULONG(1 << 11))    // Request to send
#define UARTCR_Out1     (ULONG(1 << 12))    // The Out1 modem status output
#define UARTCR_Out2     (ULONG(1 << 13))    // The Out2 modem status output
#define UARTCR_RTSEn    (ULONG(1 << 14))    // RTS hardware flow control enable
#define UARTCR_CTSEn    (ULONG(1 << 15))    // CTS hardware flow control enable
#define UARTCR_ALL      (ULONG(0x0000FF87)) // Valid (all)control bits

// UART HW control and signals
#define UART_CONTROL_LINES_MODEM_STATUS (ULONG(UARTCR_LBE |\
                                            UARTCR_DTR    |\
                                            UARTCR_RTS    |\
                                            UARTCR_Out1   |\
                                            UARTCR_Out2   |\
                                            UARTCR_RTSEn  |\
                                            UARTCR_CTSEn))


//
// PL011 Interrupt FIFO level select register (UARTIFLS) fields definition
//
#define UARTIFLS_TXIFLSEL_MASK  (ULONG(7 << 0)) // TX FIFO threshold
    typedef enum _UARTIFLS_TXIFLSEL : ULONG {
        TXIFLSEL_1_8 = (ULONG(0 << 0)), // TX FIFO <= 1/8 full
        TXIFLSEL_1_4 = (ULONG(1 << 0)), // TX FIFO <= 1/4 full
        TXIFLSEL_1_2 = (ULONG(2 << 0)), // TX FIFO <= 1/2 full
        TXIFLSEL_3_4 = (ULONG(3 << 0)), // TX FIFO <= 3/4 full
        TXIFLSEL_7_8 = (ULONG(4 << 0)), // TX FIFO <= 7/8 full
    } UARTIFLS_TXIFLSEL;
#define UARTIFLS_RXIFLSEL_MASK   (ULONG(7 << 3)) // RX FIFO threshold
    typedef enum _UARTIFLS_RXIFLSEL : ULONG {
       RXIFLSEL_1_8 = (ULONG(0 << 3)), // RX FIFO >= 1/8 full
       RXIFLSEL_1_4 = (ULONG(1 << 3)), // RX FIFO >= 1/4 full
       RXIFLSEL_1_2 = (ULONG(2 << 3)), // RX FIFO >= 1/2 full
       RXIFLSEL_3_4 = (ULONG(3 << 3)), // RX FIFO >= 3/4 full
       RXIFLSEL_7_8 = (ULONG(4 << 3)), // RX FIFO >= 7/8 full
    } UARTIFLS_RXIFLSEL;

//
// PL011 Interrupt mask set/clear register (UARTIMSC) fields definition
//
#define UARTIMSC_RIMIM  (ULONG(1 << 0))     // nUARTRI modem interrupt mask
#define UARTIMSC_CTSMIM (ULONG(1 << 1))     // CTSMIM modem interrupt mask
#define UARTIMSC_DCDMIM (ULONG(1 << 2))     // nUARTDCD modem interrupt mask
#define UARTIMSC_DSRMIM (ULONG(1 << 3))     // nUARTDSR modem interrupt mask
#define UARTIMSC_RXIM   (ULONG(1 << 4))     // Receive interrupt mask
#define UARTIMSC_TXIM   (ULONG(1 << 5))     // Transmit interrupt mask
#define UARTIMSC_RTIM   (ULONG(1 << 6))     // Receive timeout interrupt mask
#define UARTIMSC_FEIM   (ULONG(1 << 7))     // Framing error interrupt mask
#define UARTIMSC_PEIM   (ULONG(1 << 8))     // Parity error interrupt mask
#define UARTIMSC_BEIM   (ULONG(1 << 9))     // Break error interrupt mask
#define UARTIMSC_OEIM   (ULONG(1 << 10))    // Overrun error interrupt mask

// ALL interrupts
//#define UART_INTERUPPTS_ALL          (ULONG(0x000007FF))

// Modem status interrupts (without Break)
#define UART_INTERUPPTS_MODEM_STATUS (ULONG(UARTIMSC_RIMIM |\
                                           UARTIMSC_CTSMIM |\
                                           UARTIMSC_DCDMIM |\
                                           UARTIMSC_DSRMIM))

// Errors interrupts (without Break)
// #define UART_INTERUPPTS_ERRORS (ULONG(UARTIMSC_OEIM |\
                                      // UARTIMSC_PEIM |\
                                      // UARTIMSC_FEIM))

// All events (without the RX/TX)
#define UART_INTERRUPTS_EVENTS  (ULONG(UART_INTERUPPTS_ERRORS|\
                                       UART_INTERUPPTS_MODEM_STATUS))

//
// PL011 Raw interrupt status register (UARTRIS) fields definition
//
#define UARTRIS_RIMIS  (ULONG(1 << 0))  // nUARTRI modem interrupt status
#define UARTRIS_CTSMIS (ULONG(1 << 1))  // CTSMIM modem interrupt status
#define UARTRIS_DCDMIS (ULONG(1 << 2))  // nUARTDCD modem interrupt status
#define UARTRIS_DSRMIS (ULONG(1 << 3))  // nUARTDSR modem interrupt status
#define UARTRIS_RXIS   (ULONG(1 << 4))  // Receive interrupt status
#define UARTRIS_TXIS   (ULONG(1 << 5))  // Transmit interrupt status
#define UARTRIS_RTIS   (ULONG(1 << 6))  // Receive timeout interrupt status
#define UARTRIS_FEIS   (ULONG(1 << 7))  // Framing error interrupt status
#define UARTRIS_PEIS   (ULONG(1 << 8))  // Parity error interrupt status
#define UARTRIS_BEIS   (ULONG(1 << 9))  // Break error interrupt status
#define UARTRIS_OEIS   (ULONG(1 << 10)) // Overrun error interrupt status


//
// PL011 Masked interrupt status register (UARTMIS) fields definition
//
#define UARTMIS_RIMIS  (ULONG(1 << 0))  // nUARTRI modem interrupt status
#define UARTMIS_CTSMIS (ULONG(1 << 1))  // CTSMIM modem interrupt status
#define UARTMIS_DCDMIS (ULONG(1 << 2))  // nUARTDCD modem interrupt status
#define UARTMIS_DSRMIS (ULONG(1 << 3))  // nUARTDSR modem interrupt status
#define UARTMIS_RXIS   (ULONG(1 << 4))  // Receive interrupt status
#define UARTMIS_TXIS   (ULONG(1 << 5))  // Transmit interrupt status
#define UARTMIS_RTIS   (ULONG(1 << 6))  // Receive timeout interrupt status
#define UARTMIS_FEIS   (ULONG(1 << 7))  // Framing error interrupt status
#define UARTMIS_PEIS   (ULONG(1 << 8))  // Parity error interrupt status
#define UARTMIS_BEIS   (ULONG(1 << 9))  // Break error interrupt status
#define UARTMIS_OEIS   (ULONG(1 << 10)) // Overrun error interrupt status


//
// PL011 Interrupt clear register (UARTICR) fields definition
//
#define UARTICR_RIMIC  (ULONG(1 << 0))  // nUARTRI modem interrupt clear
#define UARTICR_CTSMIC (ULONG(1 << 1))  // CTSMIM modem interrupt clear
#define UARTICR_DCDMIC (ULONG(1 << 2))  // nUARTDCD modem interrupt clear
#define UARTICR_DSRMIC (ULONG(1 << 3))  // nUARTDSR modem interrupt clear
#define UARTICR_RXIC   (ULONG(1 << 4))  // Receive interrupt clear
#define UARTICR_TXIC   (ULONG(1 << 5))  // Transmit interrupt clear
#define UARTICR_RTIC   (ULONG(1 << 6))  // Receive timeout interrupt clear
#define UARTICR_FEIC   (ULONG(1 << 7))  // Framing error interrupt clear
#define UARTICR_PEIC   (ULONG(1 << 8))  // Parity error interrupt clear
#define UARTICR_BEIC   (ULONG(1 << 9))  // Break error interrupt clear
#define UARTICR_OEIC   (ULONG(1 << 10)) // Overrun error interrupt clear


//
// PL011 DMA control register (UARTDMACR) fields definition
//
#define UARTDMACR_RXDMAE    (ULONG(1 << 0))     // Receive DMA enable
#define UARTDMACR_TXDMAE    (ULONG(1 << 1))     // Transmit DMA enable
#define UARTDMACR_DMAONERR  (ULONG(1 << 2))     // DMA on error

#endif 


//×××××××××××××××××××××××××××××××××××A64 reg×××××××××××××××××××××××××××××××××××××××××××××××
//UART CLOCK Register
#define CCU_ADDRESS_BASE 0x01C20000
#define CCU_ADDRESS_SIZE 0x400
#define CCU_UART_CLOCK_GATE   0X6C
#define CCU_UART_RESET        0X2D8

//UART GPIO physical address
#define GPIO_ADDRESS_BASE 0x01C20800   //GPIO Bx~GPIO Hx
#define GPIO_ADDRESS_SIZE 0x400
#define GPIO_PB_OFFSEET  0X24
//registers offsets for A64
typedef enum _PL011_REG_FILE {
    UART_RBR          = 0x0000,//	UART_THR  	UART_DLL  //see UART_LCR  DLAB BIT.
    UART_DLH          = 0x0004,//	UART_IER    
	UART_IIR          = 0x0008,//   UART_FCR
    UART_LCR          = 0x000C,  
    UART_MCR        = 0x0010,   
    UART_LSR        = 0x0014,
    UART_MSR        = 0x0018,
    UART_SCH        = 0x001C,
    UART_USR        = 0x007C,
    UART_TFL        = 0x0080,
    UART_RFL        = 0x0084,
    UART_HALT       = 0x00a4,
    //REG_FILE_SIZE   = 0x00f0
    REG_FILE_SIZE   = 0x03ff
} PL011_REG_FILE;

#define UART_THR  0X0000
#define UART_DLL  0X0000
#define UART_IER  0X0004
#define UART_FCR  0X0008

//uart interrupt indentify register   UART_IIR
#define UART_FEN     (ULONG(3 << 6)) // Enable FIFOs  FEFLAG

//interrupt ID
#define UART_IID              (ULONG(0XF << 0)) 
#define MODEM_STATUS           0X0
#define NOINR_PEND             0X1
#define THR_EMPTY              0X2
#define RECEIVD_AVAILABLE      0X4
#define RECEIVER_LINE_STATUS   0X6
#define BUSY_DETECT            0X7
#define CHARACTER_TOUT         0XC

//uart fifo control register   UART_FCR
#define UARTIFLS_RXIFLSEL_MASK   (ULONG(3 << 6)) // RX FIFO threshold
    typedef enum _UARTIFLS_RXIFLSEL : ULONG 
	{
       RXIFLSEL_0 = (ULONG(0 << 6)), // RX FIFO  1 char in the fifo
       RXIFLSEL_1 = (ULONG(1 << 6)), // RX FIFO  1/4 full
       RXIFLSEL_2 = (ULONG(2 << 6)), // RX FIFO  1/2 full
       RXIFLSEL_3 = (ULONG(3 << 6)), // RX FIFO  -2 less than full
    } UARTIFLS_RXIFLSEL;

#define UARTIFLS_TXIFLSEL_MASK  (ULONG(3 << 4)) // TX FIFO threshold
    typedef enum _UARTIFLS_TXIFLSEL : ULONG 
	{
        TXIFLSEL_0 = (ULONG(0 << 4)), // TX FIFO  empty
        TXIFLSEL_1 = (ULONG(1 << 4)), // TX FIFO  2 char in the fifo
        TXIFLSEL_2 = (ULONG(2 << 4)), // TX FIFO  1/4 full
        TXIFLSEL_3 = (ULONG(3 << 4)), // TX FIFO  1/2 full
    } UARTIFLS_TXIFLSEL;

#define UART_DMAM       (ULONG(1 << 3))    //DMA MODE 
#define UART_XFIFOR     (ULONG(1 << 2))    //transmit FIFO Reset,clean TX FIFO empty
#define UART_REIFOR     (ULONG(1 << 1))    //receive FIFO Reset,clean RX FIFO empty
#define UART_FIFOE      (ULONG(1 << 0))    //enable FIFO ,change this bit will reset fifo both TX and RX.


//uart line control register  UART_LCR
#define UART_DLAB            (ULONG(1 << 7))     //Divisor latch access bit (1:bandrate set enable bit)
#define UART_BC              (ULONG(1 << 6))     //break control bit( 1:stop transimt  0:continue)
#define UART_ODD_PARITY      (ULONG(0 << 4))
#define UART_EVEN_PARITY     (ULONG(1 << 4))
#define PARITY_ENABLE        (ULONG(1 << 3))  //0：disable  1:enable
#define STOP_BIT             (ULONG(1 << 2))  //0:1 bit    1: 2 bit(1.5 bit when data length set 5)
#define UARTLCR_WLEN_MASK    (ULONG(3 << 0))  
   
   typedef enum _UARTLCR_WLEN : ULONG {  
       WLEN_8BITS = (ULONG(3 << 0)), // 8 bit word
       WLEN_7BITS = (ULONG(2 << 0)), // 7 bit word
       WLEN_6BITS = (ULONG(1 << 0)), // 6 bit word
       WLEN_5BITS = (ULONG(0 << 0)), // 5 bit word  
    } UARTLCR_WLEN;


//uart modem control register   UART_MCR
#define UART_SIRE      (ULONG(1 << 6))   //SIR mode enable
#define UART_FLOWCONTROL_En    (ULONG(1 << 5))    // hardware flow control enable
#define UART_AFCE     (ULONG(1 << 5))   //auto flow control enable
#define UART_LOOP     (ULONG(1 << 4))    // Loop back enable
#define UART_RTS      (ULONG(1 << 1))    //RTS: Request to send
#define UART_DTR      (ULONG(1 << 0))    //DTR: Data transmit ready

#define UARTCR_ALL    (ULONG(0xff))      //valid all contril bits

//uart line status register UART_LSR
#define UART_FIFOERR  (ULONG(1 << 7))  //(rx data error in fif0)
#define UART_TEMT     (ULONG(1 << 6))  //transmitter empty (THRE and tx shift reg both empty)
#define UART_THRE     (ULONG(1 << 5))  //TX Holding register empty 
#define UART_BI       (ULONG(1 << 4))  //break interrupt 
#define UART_FE       (ULONG(1 << 3))  //framing error
#define UART_PE       (ULONG(1 << 2))  //parity error
#define UART_OE       (ULONG(1 << 1))  //onvrrun eeror
#define UART_DR       (ULONG(1 << 0))  //data ready 

// Errors interrupts (without Break)
#define UART_INTERUPPTS_ERRORS (ULONG(UART_OE |\
                                      UART_PE |\
                                      UART_FE))
									  
									  

//uart modem status register  UART_MSR (flow control state)
#define UART_DCD  (ULONG(1 << 7))   //line state of data carrier detect
#define UART_RI   (ULONG(1 << 6))   //ring indicator
#define UART_DSR  (ULONG(1 << 5))   //line state of data set ready
#define UART_CTS  (ULONG(1 << 4))   //line state of clear to send
#define UART_DDCD  (ULONG(1 << 3))  //delta data carrier detect
#define UART_TERI  (ULONG(1 << 2))  //trailing edge ring indicator
#define UART_DDSR  (ULONG(1 << 1))  //delta data set ready
#define UART_DCTS  (ULONG(1 << 0))  //delta clear to send

// Modem status interrupts (without Break)
#define UART_INTERUPPTS_MODEM_STATUS (ULONG( UART_RI|\
                                           UART_CTS |\
                                           UART_DCD |\
                                           UART_DDSR))
										   
// All events (without the RX/TX)
#define UART_INTERRUPTS_EVENTS (ULONG( UART_INTERUPPTS_ERRORS | UART_INTERUPPTS_MODEM_STATUS ))										   

//uart scratch register   UART_SCH
#define UART_SCRATCH  (ULONG(0XFF << 0))  

//uart status register    UART_USR
#define UART_RFF   (ULONG(1 << 4))    //Receive FIFO full   0: not full  1:full
#define UART_RFNE   (ULONG(1 << 3))   //Receive FIF0 Empty    0:empty  1： not empty
#define UART_TFE   (ULONG(1 << 2))    //Transmit FIFO Empty   0:not empty 1:empty
#define UART_TFNF   (ULONG(1 << 1))   //Transmit FIFO full    0:full   1: not full
#define UART_BUSY   (ULONG(1 << 0))   //0: idle or inactive  1: busy

//uart transmit FIFO level register  UART_TFL

//uart receive FIFO Level registor UART_RFL

//uart halt TX register UART_HALT
#define SIR_RX_INVERT  (ULONG(1 << 5))  // 0: not invert   1:invert receiver signal
#define SIR_TX_INVERT  (ULONG(1 << 4))  // 0: not invert transmit pulse  1: invert
#define CHANGE_UPDATE  (ULONG(1 << 2))  //1: update trigger  (when changed LCR)
#define CHCFG_AT_BUSY  (ULONG(1 << 1))  //1: Enable change when busy
#define HALT_TX        (ULONG(1 << 0))  //0:disable    1:enable 

//
// A64 Interrupt ENBABLE register (UART_IER) fields definition
//
#define UART_INTERUPPTS_ALL    (ULONG(0x000000FF))	
#define UARTIMSC_ERBFI  (ULONG(1 << 0))     // Enable Received Data Available interrupt
#define UARTIMSC_ETBEI  (ULONG(1 << 1))     // Enable Transmit Holding Register Empty 
#define UARTIMSC_ELSI   (ULONG(1 << 2))      // Enable Receiver Line Status interrupt
#define UARTIMSC_EDSSI  (ULONG(1 << 3))     // Enable modem status interrupt
#define UARTIMSC_PTIME   (ULONG(1 << 7))     // Programmable THRE interrupt Mode enable


	
// Default PL011 supported controls based
// on the Raspberry Pi2 UART.
// On Raspberry Pi2 UART HW control signals are not 
// exposed, thus not supported.
// (Can be overwritten by inf)
//

#define PL011_DEFAULT_SUPPORTED_CONTROLS (ULONG(\
    UARTCR_UARTEN   |\
    UARTCR_LBE      |\
    UARTCR_TXE      |\
    UARTCR_RXE       \
    ))	

//
// PL011 baud rate range [BPS]
//
#define PL011_MIN_BAUD_RATE_BPS             9600  //110
//
// The following can be overwritten by registry settings.
//
#define PL011_MAX_BAUD_RATE_BPS             115200  //921600

//
// Max allowed baud rate error [percent]
//
#define PL011_MAX_BUAD_RATE_ERROR_PERCENT    1


//
// Control register update mode
//
typedef enum class _REG_UPDATE_MODE {

    INVALID = 0,
    BITMASK_SET,
    BITMASK_CLEAR,
    QUERY,
    OVERWRITE

} REG_UPDATE_MODE;


//
// PL011hw public methods
//

//
// Routine Description:
//
//  PL011HwRegAddress is called to get the virtual address of 
//  a PL011 register given the offset.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
//  RegisterOffset - The desired register offset, should be one of
//      PL011_REG_FILE values.
//
// Return Value:
//
//  The virtual address of the desired PL011 register.
//
__forceinline
volatile ULONG*
PL011HwRegAddress(
    _In_ PL011_DEVICE_EXTENSION* DevExtPtr,
    _In_range_(0, REG_FILE_SIZE) ULONG PL011RegisterOffset
    )
{
    NT_ASSERT(PL011RegisterOffset <= ULONG(PL011_REG_FILE::REG_FILE_SIZE));
    NT_ASSERT((PL011RegisterOffset & 3) == 0);

    return &DevExtPtr->PL011RegsPtr[PL011RegisterOffset / PL1011_REG_STRIDE];
}

//
// Routine Description:
//
//  PL011HwReadRegisterUlong is used to read a 32 bit register, given
//  the register virtual address.
//
// Arguments:
//
//  RegisterPtr - The virtual address of the desired 32 bit register.
//
// Return Value:
//
//  The current value of the desired 32 bit register.
//
__forceinline
ULONG
PL011HwReadRegisterUlong(
    _In_ volatile ULONG* RegisterPtr
    )
{
    return READ_REGISTER_ULONG(RegisterPtr);
}

//
// Routine Description:
//
//  PL011HwReadRegisterUlongNoFence is used to read a 32 bit register, given
//  the register virtual address.
//  It uses a non-fenced access, which is faster and can be used for read-only 
//  processes like reading from FIFOs, etc.
//
// Arguments:
//
//  RegisterPtr - The virtual address of the desired 32 bit register.
//
// Return Value:
//
//  The current value of the desired 32 bit register.
//
__forceinline
ULONG
PL011HwReadRegisterUlongNoFence(
    _In_ volatile ULONG* RegisterPtr
    )
{
    return READ_REGISTER_NOFENCE_ULONG(RegisterPtr);
}

//
// Routine Description:
//
//  PL011HwWriteRegisterUlong is used to write a 32 bit register, given
//  the register virtual address, and the new value.
//
// Arguments:
//
//  RegisterPtr - The virtual address of the desired 32 bit register.
//
//  Value - the new value to be written to the desired register.
//
// Return Value:
//
__forceinline
VOID
PL011HwWriteRegisterUlong(
    _In_ volatile ULONG* RegisterPtr,
    _In_ ULONG Value
    )
{
    #if IS_DONT_CHANGE_HW

        UNREFERENCED_PARAMETER(RegisterPtr);
        UNREFERENCED_PARAMETER(Value);

    #else   // IS_DONT_CHANGE_HW

        WRITE_REGISTER_ULONG(RegisterPtr, Value);

    #endif  // !IS_DONT_CHANGE_HW
}

//
// Routine Description:
//
//  PL011HwWriteRegisterUlong is used to write a 32 bit register, given
//  the register virtual address, and the new value.
//  It uses a non-fenced access, which is faster and can be used for write-only 
//  processes like writing to FIFOs, etc.
//
// Arguments:
//
//  RegisterPtr - The virtual address of the desired 32 bit register.
//
//  Value - the new value to be written to the desired register.
//
// Return Value:
//
__forceinline
VOID
PL011HwWriteRegisterUlongNoFence(
    _In_ volatile ULONG* RegisterPtr,
    _In_ ULONG Value
    )
{
    #if IS_DONT_CHANGE_HW

        UNREFERENCED_PARAMETER(RegisterPtr);
        UNREFERENCED_PARAMETER(Value);

    #else   // IS_DONT_CHANGE_HW

        WRITE_REGISTER_NOFENCE_ULONG(RegisterPtr, Value);

    #endif  // !IS_DONT_CHANGE_HW
}

//
// Routine Description:
//
//  PL011HwClearInterrupts is called to clear (ACK) 
//  pending interrupts.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
//  InterruptsMask - The interrupt mask to clear.
//
// Return Value:
//

// __forceinline
// VOID
// PL011HwClearInterrupts(
    // _In_ PL011_DEVICE_EXTENSION* DevExtPtr,
    // _In_ ULONG InterruptsMask
    // )
// {
    // NT_ASSERT((InterruptsMask & ~UART_INTERUPPTS_ALL) == 0);

    // PL011HwWriteRegisterUlong(
        // PL011HwRegAddress(DevExtPtr, UARTICR),
        // InterruptsMask
        // );
// }

//
// Routine Description:
//
//  PL011HwClearRxErros is called to clear current RX errors.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
// Return Value:
//
// __forceinline
// VOID
// PL011HwClearRxErros(
    // _In_ PL011_DEVICE_EXTENSION* DevExtPtr
    // )
// {
    // PL011HwWriteRegisterUlong(
        // PL011HwRegAddress(DevExtPtr, UARTRSR_ECR),
        // 0 // Value not important
        // );
	   // PL011HwWriteRegisterUlong(
        // PL011HwRegAddress(DevExtPtr, UART_LSR),
        // 0 
        // );
// }

//
// Routine Description:
//
//  PL011HwIsRxFifoEmpty is called to check if RX FIFO
//  is empty.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
// Return Value:
// 
//  TRUE if RX FIFO is empty otherwise FALSE
//
__forceinline
BOOLEAN
PL011HwIsRxFifoEmpty(
    _In_ PL011_DEVICE_EXTENSION* DevExtPtr
    )
{
    // return (PL011HwReadRegisterUlong(
        // PL011HwRegAddress(DevExtPtr, UARTFR)
        // ) & UARTFR_RXFE) != 0;
	   return (PL011HwReadRegisterUlong(
        PL011HwRegAddress(DevExtPtr, UART_USR)
        ) & UART_RFNE) == 0;
}

//
// Routine Description:
//
//  PL011HwIsTxBusy is called to check if TX is busy sending data.
//  TX is busy if TX FIFO is not empty and TX status is marked 
//  as busy.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
// Return Value:
// 
//  TRUE if TX is busy otherwise FALSE
//
__forceinline
BOOLEAN
PL011HwIsTxBusy(
    _In_ PL011_DEVICE_EXTENSION* DevExtPtr
    )
{
    ULONG regUARTFR = PL011HwReadRegisterUlong(
        PL011HwRegAddress(DevExtPtr, UART_USR)
        );
    return ((regUARTFR & UART_TFE) == 0) ||
        ((regUARTFR & UART_BUSY) != 0);
}


//UART IS BUSY 
__forceinline
BOOLEAN
PL011HwIsBusy(
    _In_ PL011_DEVICE_EXTENSION* DevExtPtr
    )
{
		
    ULONG regUARTFR = PL011HwReadRegisterUlong(
        PL011HwRegAddress(DevExtPtr, UART_USR)
        );
    return ((regUARTFR & UART_BUSY) != 0);
}

//
// Routine Description:
//
//  PL011HwIsTxFifoEmpty is called to check if TX FIFO
//  is empty.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
// Return Value:
// 
//  TRUE if TX FIFO is empty otherwise FALSE
//
__forceinline
BOOLEAN
PL011HwIsTxFifoEmpty(
    _In_ PL011_DEVICE_EXTENSION* DevExtPtr
    )
{
      return (PL011HwReadRegisterUlong(
        PL011HwRegAddress(DevExtPtr, UART_USR)
        ) & UART_TFE) != 0;
}

//
// Routine Description:
//
//  PL011HwIsTxFifoFull is called to check if TX FIFO
//  is full.
//
// Arguments:
//
//  DevExtPtr - Address of the PL011 device extension.
//
// Return Value:
// 
//  TRUE if TX FIFO is not full otherwise FALSE
//
__forceinline
BOOLEAN
PL011HwIsTxFifoFull(
    _In_ PL011_DEVICE_EXTENSION* DevExtPtr
    )
{
	return (PL011HwReadRegisterUlong(
        PL011HwRegAddress(DevExtPtr, UART_USR)
        ) & UART_TFNF) == 0;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PL011HwInitController(
    _In_ WDFDEVICE WdfDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwStopController(
    _In_ WDFDEVICE WdfDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwGetSupportedBaudRates(
    _In_ WDFDEVICE WdfDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwUartControl(
    _In_ WDFDEVICE WdfDevice,
    _In_ ULONG UartControlMask,
    _In_ REG_UPDATE_MODE RegUpdateMode,
    _Out_opt_ ULONG* OldUartControlPtr
    );
	
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwSetFifoThreshold(
    _In_ WDFDEVICE WdfDevice,
    _In_ UARTIFLS_RXIFLSEL RxInterruptTrigger,
    _In_ UARTIFLS_TXIFLSEL TxInterruptTrigger
    );

_When_(IsIsrSafe == 1, _IRQL_requires_max_(DISPATCH_LEVEL))
VOID
PL011HwMaskInterrupts(
    _In_ WDFDEVICE WdfDevice,
    _In_ ULONG InterruptMask,
    _In_ BOOLEAN IsMaskInterrupts,
    _In_ BOOLEAN IsIsrSafe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PL011HwSetBaudRate(
    _In_ WDFDEVICE WdfDevice,
    _In_ ULONG BaudRateBPS
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PL011HwSetFlowControl(
    _In_ WDFDEVICE WdfDevice,
    _In_ const SERIAL_HANDFLOW* SerialFlowControlPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PL011HwSetLineControl(
    _In_ WDFDEVICE WdfDevice,
    _In_ const SERIAL_LINE_CONTROL* SerialLineControlPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwEnableFifos(
    _In_ WDFDEVICE WdfDevice,
    _In_ BOOLEAN IsEnable
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PL011HwSetModemControl(
    _In_ WDFDEVICE WdfDevice,
    _In_ UCHAR ModemControl
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PL011HwGetModemControl(
    _In_ WDFDEVICE WdfDevice,
    _Out_ UCHAR* ModemControlPtr
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwSetBreak(
    _In_ WDFDEVICE WdfDevice,
    _In_ BOOLEAN IsBreakON
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwRegsDump(
    _In_ WDFDEVICE WdfDevice
    );
	
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PL011HwSetDivMode(
    WDFDEVICE WdfDevice,
    BOOLEAN IsON
    );

//
// PL011hw private methods
//
#ifdef _PL011_HW_CPP_

#endif //_PL011_HW_CPP_


WDF_EXTERN_C_END

#endif // !_PL011_HW_H_
