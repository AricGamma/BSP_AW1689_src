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

#include  "usbc_i.h"
#include "../include/platform.h"

/*
*******************************************************************************
*                     usb_open_clock
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_open_clock(void)
{
#if 0
	u32 reg_value = 0;

#ifdef FPGA_PLATFORM
	//change interfor on  fpga 
	reg_value = USBC_Readl(SUNXI_SYSCRL_BASE+0x04);
	reg_value |= 0x01;
	USBC_Writel(reg_value,SUNXI_SYSCRL_BASE+0x04);
#endif

	//Enable module clock for USB phy0
	reg_value = readl(SUNXI_CCM_BASE + 0xcc);
	reg_value |= (1 << 0) | (1 << 8);
	writel(reg_value, (SUNXI_CCM_BASE + 0xcc));
	//delay some time
	__msdelay(10);

	//Gating AHB clock for USB_phy0
	reg_value = readl(SUNXI_CCM_BASE + 0x60);
	reg_value |= (1 << 23);
	writel(reg_value, (SUNXI_CCM_BASE + 0x60));

	//delay to wati SIE stable
	__msdelay(10);

	reg_value = readl(SUNXI_CCM_BASE + 0x2C0);
	reg_value |= (1 << 23);
	writel(reg_value, (SUNXI_CCM_BASE + 0x2C0));
	__msdelay(10);

	reg_value = readl(SUNXI_USBOTG_BASE + 0x420);
	reg_value |= (0x01 << 0);
	writel(reg_value, (SUNXI_USBOTG_BASE + 0x420));
	__msdelay(10);

	reg_value = readl(SUNXI_USBOTG_BASE + 0x410);
	reg_value &= ~(0x01 << 1);
	writel(reg_value, (SUNXI_USBOTG_BASE + 0x410));
	__msdelay(10);
#endif
	return 0;
}
/*
*******************************************************************************
*                     usb_op_clock
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_close_clock(void)
{
#if 0
    u32 reg_value = 0;

    /* AHB reset */
    reg_value = readl(SUNXI_CCM_BASE + 0x2C0);
    reg_value &= ~(1 << 23);
    writel(reg_value, (SUNXI_CCM_BASE + 0x2C0));
    __msdelay(10);

	reg_value = readl(SUNXI_CCM_BASE + 0x60);
	reg_value &= ~(1 << 23);
	writel(reg_value, (SUNXI_CCM_BASE + 0x60));
	__msdelay(10);

	reg_value = readl(SUNXI_CCM_BASE + 0xcc);
	reg_value &= ~((1 << 0) | (1 << 8));
	writel(reg_value, (SUNXI_CCM_BASE + 0xcc));
	__msdelay(10);
#endif
	return 0;
}
#if 0
/*
*******************************************************************************
*                     usb_probe_vbus_type
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_probe_vbus_type(void)
{
	__u32 base_reg_val;
	__u32 reg_val = 0;
	__u32 dp, dm = 0;
	__u32 dpdm_det[6];
	__u32 dpdm_ret = 0;
	int   i =0 ;

	reg_val = readl(SUNXI_USBOTG_BASE + USBC_REG_o_ISCR);
	base_reg_val = reg_val;
	reg_val |= (1 << 16) | (1 << 17);
	writel(reg_val, SUNXI_USBOTG_BASE + USBC_REG_o_ISCR);

	__msdelay(2);
	for(i=0;i<6;i++)
	{
		reg_val = readl(SUNXI_USBOTG_BASE + USBC_REG_o_ISCR);
 	 	dp = (reg_val >> 26) & 0x01;
 	 	dm = (reg_val >> 27) & 0x01;

 	 	dpdm_det[i] = (dp << 1) | dm;
 	 	dpdm_ret += dpdm_det[i];

		__msdelay(1);
	}
	writel(base_reg_val, SUNXI_USBOTG_BASE + USBC_REG_o_ISCR);

  	if(dpdm_ret > 12)
  	{
  		return 1;			//DC
  	}
  	else
  	{
  		return 0;			//PC
  	}
}
#endif
