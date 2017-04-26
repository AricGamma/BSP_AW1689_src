
/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minitopo.h

Abstract:

    Declaration of topology miniport.

--*/

#ifndef _SYSVAD_MINTOPO_H_
#define _SYSVAD_MINTOPO_H_

#include "basetopo.h"

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CMiniportTopology 
//   

class CMiniportTopology : 
    public CMiniportTopologySYSVAD,
    public IMiniportTopology,
    public CUnknown
{
  private:
    eDeviceType             m_DeviceType;
    union {
        PVOID               m_DeviceContext;
    };

public:
    DECLARE_STD_UNKNOWN();
    CMiniportTopology
    (
        _In_opt_    PUNKNOWN                UnknownOuter,
        _In_        PCFILTER_DESCRIPTOR    *FilterDesc,
        _In_        USHORT                  DeviceMaxChannels,
        _In_        eDeviceType             DeviceType, 
        _In_opt_    PVOID                   DeviceContext
    )
    : CUnknown(UnknownOuter),
      CMiniportTopologySYSVAD(FilterDesc, DeviceMaxChannels),
      m_DeviceType(DeviceType),
      m_DeviceContext(DeviceContext)
    {

    }

    ~CMiniportTopology();

    IMP_IMiniportTopology;

    NTSTATUS PropertyHandlerJackDescription
    (
        _In_        PPCPROPERTY_REQUEST                      PropertyRequest,
        _In_        ULONG                                    cJackDescriptions,
        _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *  JackDescriptions
    );

    NTSTATUS PropertyHandlerJackDescription2
    ( 
        _In_        PPCPROPERTY_REQUEST                      PropertyRequest,
        _In_        ULONG                                    cJackDescriptions,
        _In_reads_(cJackDescriptions) PKSJACK_DESCRIPTION *  JackDescriptions,
        _In_        DWORD                                    JackCapabilities
    );
    PVOID GetDeviceContext() { return m_DeviceContext;  }
};

typedef CMiniportTopology *PCMiniportTopology;

NTSTATUS CMiniportTopology_EventHandler_JackState
(
    _In_  PPCEVENT_REQUEST EventRequest
);

#endif // _SYSVAD_MINTOPO_H_

