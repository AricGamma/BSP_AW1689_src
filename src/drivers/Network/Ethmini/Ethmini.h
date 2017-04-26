/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

   Ethmini.H

Abstract:

    This module collects all the headers needed to compile the netvmin6 sample.

--*/


#ifndef _ETHMIN_H
#define _ETHMIN_H


#include <ndis.h>


// Tell the analysis tools that this code should be treated as a kernel driver.
_Analysis_mode_(_Analysis_code_type_kernel_driver_);

#include "trace.h"
#include "hardware.h"
#include "hw_dma.h"
#include "miniport.h"
#include "adapter.h"
#include "hw_phy.h"
#include "hw_Mac.h"
#include "mphal.h"
#include "hw_isr.h"
#include "tcbrcb.h"
#include "datapath.h"
#include "ctrlpath.h"

#define CHKSTATUS(func, message)\
	Status = func;\
	if(Status != NDIS_STATUS_SUCCESS)\
	{\
		__debugbreak();\
		goto Exit;\
	}

#endif // _ETHMIN_H

