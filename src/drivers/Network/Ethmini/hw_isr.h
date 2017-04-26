#ifndef _HWISR_H
#define _HWISR_H

/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    hw_isr.h

Abstract:
    Contains defines for interrupt processing in the HW layer
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    09-04-2007    Created

Notes:

--*/
#pragma once

struct _ADAPTER_HW;

NDIS_STATUS
HwRegisterInterrupt(__in  struct _ADAPTER_HW  *Hw);

MINIPORT_ISR HWInterrupt;

// Receive side throttling compatible interrupt DPC
MINIPORT_INTERRUPT_DPC HWInterruptDPC;

MINIPORT_ENABLE_INTERRUPT HwEnableInterrupt;

MINIPORT_DISABLE_INTERRUPT HwDisableInterrupt;

#endif
