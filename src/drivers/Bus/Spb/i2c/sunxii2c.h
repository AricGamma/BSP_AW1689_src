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

//
// Includes for hardware register definitions.
//

#ifndef _SUNXII2C_H_
#define _SUNXII2C_H_

#include "hw.h"

//
// Skeleton I2C controller registers.
//

/*
AddrReg:	31:8bit reserved, 7-1bit for slave addr, 0bit for GCE
XaddrReg:	31:8bit reserved,7-0bit for second addr in 10bit addr 
DatarReg:	31:8bit reserved, 7-0bit send or receive data byte	
CtlReg:		INT_EN,BUS_EN,M_STA,INT_FLAG,A_ACK
StatReg:	28 interrupt types + 0xF8 normal type = 29
ClkReg:		31:7bit reserved,6-3bit,CLK_M,2-0bit CLK_N
SrstReg:	31:1bit reserved;0bit,write 1 to clear 0.
EfrReg:		31:2bit reserved,1:0 bit data byte follow read comand	
LcrReg:		31:6bits reserved  5:0bit for sda&scl control
DvfsReg:	31:3bits reserved  2:0bit for dvfs control
*/
typedef struct SUNXII2C_REGISTERS
{
    // TODO: Update this register structure to match the
    //       register mapping of the controller hardware.

    __declspec(align(4)) HWREG<ULONG>  AddrReg;
    __declspec(align(4)) HWREG<ULONG>  XaddrReg;
	__declspec(align(4)) HWREG<ULONG>  DatarReg;
	__declspec(align(4)) HWREG<ULONG>  CtlReg;
	__declspec(align(4)) HWREG<ULONG>  StatReg;
	__declspec(align(4)) HWREG<ULONG>  ClkReg;
	__declspec(align(4)) HWREG<ULONG>  SrstReg;
	__declspec(align(4)) HWREG<ULONG>  EfrReg;
	__declspec(align(4)) HWREG<ULONG>  LcrReg;
	__declspec(align(4)) HWREG<ULONG>  DvfsReg;

}
SUNXII2C_REGISTERS, *PSUNXII2C_REGISTERS;

// TODO: Update the following defines to match the bit
//       functionalities of each register.

//
// TWI address register
//
#define TWI_GCE_EN      	(0x1 <<0) /* general call address enable for slave mode */
#define TWI_ADDR_MASK   	(0x7f<<1) /* 7:1bits */
/* 31:8bits reserved*/

//
//TWI extend address register
//

#define TWI_XADDR_MASK  (0xff) /* 7:0bits for extend slave address */
/* 31:8bits reserved */

//
//TWI Data register default is 0x0000_0000 
//
#define TWI_DATA_MASK   (0xff) /* 7:0bits for send or received */

//
//TWI Control Register Bit Fields & Masks, default value: 0x0000_0000
//
/* 1:0 bits reserved */
#define TWI_CTL_ACK		(0x1<<2) /* set 1 to send A_ACK,then low level on SDA */
#define TWI_CTL_INTFLG	(0x1<<3) /* INT_FLAG,interrupt status flag: set '1' when interrupt coming */
#define TWI_CTL_STP		(0x1<<4) /* M_STP,Automatic clear 0 */
#define TWI_CTL_STA		(0x1<<5) /* M_STA,atutomatic clear 0 */
#define TWI_CTL_BUSEN	(0x1<<6) /* BUS_EN, master mode should be set 1.*/
#define TWI_CTL_INTEN	(0x1<<7) /* INT_EN */
/* 31:8 bit reserved */

//
//TWI Clock Register Bit Fields & Masks,default value:0x0000_0000
//
/*
Fin is APB CLOCK INPUT;
Fsample = F0 = Fin/2^CLK_N;
F1 = F0/(CLK_M+1);

Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10);
Foscl is clock SCL;standard mode:100KHz or fast mode:400KHz
*/
#define TWI_CLK_DIV_M		(0xF<<3) /* 6:3bit  */
#define TWI_CLK_DIV_N		(0x7<<0) /* 2:0bit */


//
//TWI Soft Reset Register Bit Fields & Masks
//
#define TWI_SRST_SRST		(0x1<<0) /* write 1 to clear 0, when complete soft reset clear 0 */

//
//TWI Enhance Feature Register Bit Fields & Masks
//default -- 0x0
//
#define TWI_EFR_MASK        (0x3<<0)/* 00:no,01: 1byte, 10:2 bytes, 11: 3bytes */
#define TWI_EFR_WARC_0      (0x0<<0)
#define TWI_EFR_WARC_1      (0x1<<0)
#define TWI_EFR_WARC_2      (0x2<<0)
#define TWI_EFR_WARC_3      (0x3<<0)

//
//twi line control register -default value: 0x0000_003a
//
#define TWI_LCR_SDA_EN          (0x01<<0) 	/* SDA line state control enable ,1:enable;0:disable */
#define TWI_LCR_SDA_CTL         (0x01<<1) 	/* SDA line state control bit, 1:high level;0:low level */
#define TWI_LCR_SCL_EN          (0x01<<2) 	/* SCL line state control enable ,1:enable;0:disable */
#define TWI_LCR_SCL_CTL         (0x01<<3) 	/* SCL line state control bit, 1:high level;0:low level */
#define TWI_LCR_SDA_STATE_MASK  (0x01<<4)   /* current state of SDA,readonly bit */
#define TWI_LCR_SCL_STATE_MASK  (0x01<<5)   /* current state of SCL,readonly bit */
/* 31:6bits reserved */
#define TWI_LCR_IDLE_STATUS     (0x3a)

//
//TWI Status Register Bit Fields & Masks
//
#define TWI_STAT_MASK                   (0xff)
/* 7:0 bits use only,default is 0xF8 */
#define TWI_STAT_BUS_ERR				(0x00) 	/* BUS ERROR */
/* Master mode use only */
#define TWI_STAT_TX_STA					(0x08) 	/* START condition transmitted */
#define TWI_STAT_TX_RESTA				(0x10) 	/* Repeated START condition transmitted */
#define TWI_STAT_TX_AW_ACK				(0x18) 	/* Address+Write bit transmitted, ACK received */
#define TWI_STAT_TX_AW_NAK				(0x20) 	/* Address+Write bit transmitted, ACK not received */
#define TWI_STAT_TXD_ACK				(0x28) 	/* data byte transmitted in master mode,ack received */
#define TWI_STAT_TXD_NAK				(0x30) 	/* data byte transmitted in master mode ,ack not received */
#define TWI_STAT_ARBLOST				(0x38) 	/* arbitration lost in address or data byte */
#define TWI_STAT_TX_AR_ACK				(0x40) 	/* Address+Read bit transmitted, ACK received */
#define TWI_STAT_TX_AR_NAK				(0x48) 	/* Address+Read bit transmitted, ACK not received */
#define TWI_STAT_RXD_ACK				(0x50) 	/* data byte received in master mode ,ack transmitted */
#define TWI_STAT_RXD_NAK				(0x58) 	/* date byte received in master mode,not ack transmitted */
/* Slave mode use only */
#define TWI_STAT_RXWS_ACK				(0x60) 	/* Slave address+Write bit received, ACK transmitted */
#define TWI_STAT_ARBLOST_RXWS_ACK		(0x68)
#define TWI_STAT_RXGCAS_ACK				(0x70) 	/* General Call address received, ACK transmitted */
#define TWI_STAT_ARBLOST_RXGCAS_ACK		(0x78)
#define TWI_STAT_RXDS_ACK				(0x80)
#define TWI_STAT_RXDS_NAK				(0x88)
#define TWI_STAT_RXDGCAS_ACK			(0x90)
#define TWI_STAT_RXDGCAS_NAK			(0x98)
#define TWI_STAT_RXSTPS_RXRESTAS		(0xA0)
#define TWI_STAT_RXRS_ACK				(0xA8)

#define TWI_STAT_ARBLOST_SLAR_ACK       (0xB0)

/* 10bit Address, second part of address */
#define TWI_STAT_TX_SAW_ACK             (0xD0) 	/* Second Address byte+Write bit transmitted,ACK received */
#define TWI_STAT_TX_SAW_NAK             (0xD8) 	/* Second Address byte+Write bit transmitted,ACK not received */

#define TWI_STAT_IDLE					(0xF8) 	/* No relevant status infomation,INT_FLAG = 0 */

/* status or interrupt source */
/*------------------------------------------------------------------------------
 Code   Status
 00h    Bus error
 08h    START condition transmitted
 10h    Repeated START condition transmitted
 18h    Address + Write bit transmitted, ACK received
 20h    Address + Write bit transmitted, ACK not received
 28h    Data byte transmitted in master mode, ACK received
 30h    Data byte transmitted in master mode, ACK not received
 38h    Arbitration lost in address or data byte
 40h    Address + Read bit transmitted, ACK received
 48h    Address + Read bit transmitted, ACK not received
 50h    Data byte received in master mode, ACK transmitted
 58h    Data byte received in master mode, not ACK transmitted
 60h    Slave address + Write bit received, ACK transmitted
 68h    Arbitration lost in address as master, slave address + Write bit received, ACK transmitted
 70h    General Call address received, ACK transmitted
 78h    Arbitration lost in address as master, General Call address received, ACK transmitted
 80h    Data byte received after slave address received, ACK transmitted
 88h    Data byte received after slave address received, not ACK transmitted
 90h    Data byte received after General Call received, ACK transmitted
 98h    Data byte received after General Call received, not ACK transmitted
 A0h    STOP or repeated START condition received in slave mode
 A8h    Slave address + Read bit received, ACK transmitted
 B0h    Arbitration lost in address as master, slave address + Read bit received, ACK transmitted
 B8h    Data byte transmitted in slave mode, ACK received
 C0h    Data byte transmitted in slave mode, ACK not received
 C8h    Last byte transmitted in slave mode, ACK received
 D0h    Second Address byte + Write bit transmitted, ACK received
 D8h    Second Address byte + Write bit transmitted, ACK not received
 F8h    No relevant status information or no interrupt
-----------------------------------------------------------------------------*/

//
// TWI mode select 
//
#define TWI_MASTER_MODE     (1)
#define TWI_SLAVE_MODE      (0)	/* seldom use */


//
//I2C transfer status
//

#define I2C_XFER_IDLE		(0x1)
#define I2C_XFER_START		(0x2)
#define	I2C_XFER_RUNNING    (0x4)

// TODO: Define other controller-specific values.

#define SI2C_MAX_TRANSFER_LENGTH            0x00001000

// TODO: Remove these generic error defines in favor
//       of real register bit mappings defined above.

#define SI2C_STATUS_ADDRESS_NACK            0x00000000
#define SI2C_STATUS_DATA_NACK               0x00000000
#define SI2C_STATUS_GENERIC_ERROR           0x00000000


#define DELAY_ONE_MICROSECOND   (-10)  
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND * 1000)

//
// Register evaluation functions.
//

FORCEINLINE
bool
TestAnyBits(
    _In_ ULONG V1,
    _In_ ULONG V2
    )
{
    return (V1 & V2) != 0;
}

FORCEINLINE
bool
TestAllBits(
    _In_ ULONG V1,
    _In_ ULONG V2
    )
{
    return ((V1 & V2) == V2);
}

#endif
