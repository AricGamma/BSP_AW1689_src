/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    common.cpp

Abstract:

    Implementation of the AdapterCommon class. 

--*/

#pragma warning (disable : 4127)

#include <initguid.h>
#include <sysvad.h>
#include "hw.h"
#include "savedata.h"
#include "IHVPrivatePropertySet.h"
#include "simple.h"

//-----------------------------------------------------------------------------
// CSaveData statics
//-----------------------------------------------------------------------------

PSAVEWORKER_PARAM       CSaveData::m_pWorkItems = NULL;
PDEVICE_OBJECT          CSaveData::m_pDeviceObject = NULL;

///////////////////////////////////////////////////////////////////////////////
// CAdapterCommon
//   

#define I2S_D0_READY_EVENT_INDEX 0
#define CODEC_D0_READY_EVENT_INDEX 1
#define DEVICE_D3_READY_EVENT_COUNT 2

#define DEVICE_D3_READY_WAIT_TIMEOUT_MS 10000 // 10s

NTSTATUS I2SPowerDownCompletionCallbackRoutine(_In_ PVOID pContext);
NTSTATUS CodecPowerDownCompletionCallbackRoutine(_In_ PVOID pContext);

class CAdapterCommon : 
    public IAdapterCommon,
    public IAdapterPowerManagement,
    public CUnknown    
{
    private:
        PSERVICEGROUP           m_pServiceGroupWave;
        PDEVICE_OBJECT          m_pDeviceObject;
        PDEVICE_OBJECT          m_pPhysicalDeviceObject;
        WDFDEVICE               m_WdfDevice;            // Wdf device. 
        DEVICE_POWER_STATE      m_PowerState;  

        PCSYSVADHW              m_pHW;                  // Virtual SYSVAD HW object
        PPORTCLSETWHELPER       m_pPortClsEtwHelper;

        static LONG             m_AdapterInstances;     // # of adapter objects.

        DWORD                   m_dwIdleRequests;

		I2S_FUNCTION_INTERFACE   m_I2sFunctionInterface;   // I2S remote interface
		CODEC_FUNCTION_INTERFACE m_CodecFunctionInterface; // Codec remote interface

	public:
		KEVENT m_I2SD3ReadyEvent;
		KEVENT m_CodecD3ReadyEvent;
		PKEVENT m_pDeviceD3ReadyEvent[DEVICE_D3_READY_EVENT_COUNT];

    public:
        //=====================================================================
        // Default CUnknown
        DECLARE_STD_UNKNOWN();
        DEFINE_STD_CONSTRUCTOR(CAdapterCommon);
        ~CAdapterCommon();

        //=====================================================================
        // Default IAdapterPowerManagement
        IMP_IAdapterPowerManagement;

        //=====================================================================
        // IAdapterCommon methods      

        STDMETHODIMP_(NTSTATUS) Init
        (   
            _In_  PDEVICE_OBJECT  DeviceObject
        );

        STDMETHODIMP_(PDEVICE_OBJECT)   GetDeviceObject(void);
        
        STDMETHODIMP_(PDEVICE_OBJECT)   GetPhysicalDeviceObject(void);
        
        STDMETHODIMP_(WDFDEVICE)        GetWdfDevice(void);

		STDMETHODIMP_(PI2S_FUNCTION_INTERFACE) GetI2sFunctionInterface(void);

		STDMETHODIMP_(PCODEC_FUNCTION_INTERFACE) GetCodecFunctionInterface(void);

        STDMETHODIMP_(void)     SetWaveServiceGroup
        (   
            _In_  PSERVICEGROUP   ServiceGroup
        );

        STDMETHODIMP_(BOOL)     bDevSpecificRead();

        STDMETHODIMP_(void)     bDevSpecificWrite
        (
            _In_  BOOL            bDevSpecific
        );
        STDMETHODIMP_(INT)      iDevSpecificRead();

        STDMETHODIMP_(void)     iDevSpecificWrite
        (
            _In_  INT             iDevSpecific
        );
        STDMETHODIMP_(UINT)     uiDevSpecificRead();

        STDMETHODIMP_(void)     uiDevSpecificWrite
        (
            _In_  UINT            uiDevSpecific
        );

        STDMETHODIMP_(BOOL)     MixerMuteRead
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel
        );

        STDMETHODIMP_(void)     MixerMuteWrite
        (
            _In_  ULONG           Index,
            _In_  ULONG           Channel,
            _In_  BOOL            Value
        );

        STDMETHODIMP_(ULONG)    MixerMuxRead(void);

        STDMETHODIMP_(void)     MixerMuxWrite
        (
            _In_  ULONG           Index
        );

        STDMETHODIMP_(void)     MixerReset(void);

        STDMETHODIMP_(LONG)     MixerVolumeRead
        ( 
            _In_  ULONG           Index,
            _In_  ULONG           Channel
        );

        STDMETHODIMP_(void)     MixerVolumeWrite
        ( 
            _In_  ULONG           Index,
            _In_  ULONG           Channel,
            _In_  LONG            Value 
        );

        STDMETHODIMP_(LONG)     MixerPeakMeterRead
        ( 
            _In_  ULONG           Index,
            _In_  ULONG           Channel
        );

        STDMETHODIMP_(NTSTATUS) WriteEtwEvent 
        ( 
            _In_ EPcMiniportEngineEvent    miniportEventType,
            _In_ ULONGLONG      ullData1,
            _In_ ULONGLONG      ullData2,
            _In_ ULONGLONG      ullData3,
            _In_ ULONGLONG      ullData4
        );

        STDMETHODIMP_(VOID)     SetEtwHelper 
        ( 
            PPORTCLSETWHELPER _pPortClsEtwHelper
        );
        
        STDMETHODIMP_(NTSTATUS) InstallSubdevice
        ( 
            _In_opt_        PIRP                                        Irp,
            _In_            PWSTR                                       Name,
            _In_            REFGUID                                     PortClassId,
            _In_            REFGUID                                     MiniportClassId,
            _In_opt_        PFNCREATEMINIPORT                           MiniportCreate,
            _In_            ULONG                                       cPropertyCount,
            _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY   * pProperties,
            _In_opt_        PVOID                                       DeviceContext,
            _In_            PENDPOINT_MINIPAIR                          MiniportPair,
            _In_opt_        PRESOURCELIST                               ResourceList,
            _In_            REFGUID                                     PortInterfaceId,
            _Out_opt_       PUNKNOWN                                  * OutPortInterface,
            _Out_opt_       PUNKNOWN                                  * OutPortUnknown,
            _Out_opt_       PUNKNOWN                                  * OutMiniportUnknown
        );
        
        STDMETHODIMP_(NTSTATUS) UnregisterSubdevice
        (
            _In_opt_ PUNKNOWN               UnknownPort
        );
        
        STDMETHODIMP_(NTSTATUS) ConnectTopologies
        (
            _In_ PUNKNOWN                   UnknownTopology,
            _In_ PUNKNOWN                   UnknownWave,
            _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
            _In_ ULONG                      PhysicalConnectionCount
        );
        
        STDMETHODIMP_(NTSTATUS) DisconnectTopologies
        (
            _In_ PUNKNOWN                   UnknownTopology,
            _In_ PUNKNOWN                   UnknownWave,
            _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
            _In_ ULONG                      PhysicalConnectionCount
        );
        
        STDMETHODIMP_(NTSTATUS) InstallEndpointFilters
        (
            _In_opt_    PIRP                Irp, 
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_opt_    PVOID               DeviceContext,
            _Out_opt_   PUNKNOWN *          UnknownTopology,
            _Out_opt_   PUNKNOWN *          UnknownWave,
            _Out_opt_   PUNKNOWN *          UnknownMiniportTopology,
            _Out_opt_   PUNKNOWN *          UnknownMiniportWave
        );
        
        STDMETHODIMP_(NTSTATUS) RemoveEndpointFilters
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_opt_    PUNKNOWN            UnknownTopology,
            _In_opt_    PUNKNOWN            UnknownWave
        );

        STDMETHODIMP_(NTSTATUS) GetFilters
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _Out_opt_   PUNKNOWN            *UnknownTopologyPort,
            _Out_opt_   PUNKNOWN            *UnknownTopologyMiniport,
            _Out_opt_   PUNKNOWN            *UnknownWavePort,
            _Out_opt_   PUNKNOWN            *UnknownWaveMiniport
        );

        STDMETHODIMP_(NTSTATUS) SetIdlePowerManagement
        (
            _In_        PENDPOINT_MINIPAIR  MiniportPair,
            _In_        BOOL                bEnabled
        );

		STDMETHODIMP_(NTSTATUS) SetPowerDownCompletionCallback
		(
			THIS_
		);

		STDMETHODIMP_(NTSTATUS) SetPowerState
		(
			THIS_
			_In_ DEVICE_POWER_STATE PowerState
		);

        STDMETHODIMP_(VOID) Cleanup();
        
        //=====================================================================
        // friends
        friend NTSTATUS         NewAdapterCommon
        ( 
            _Out_       PUNKNOWN *              Unknown,
            _In_        REFCLSID,
            _In_opt_    PUNKNOWN                UnknownOuter,
            _When_((PoolType & NonPagedPoolMustSucceed) != 0,
                __drv_reportError("Must succeed pool allocations are forbidden. "
                        "Allocation failures cause a system crash"))
            _In_        POOL_TYPE               PoolType 
        );

    private:

    LIST_ENTRY m_SubdeviceCache;

    NTSTATUS GetCachedSubdevice
    (
        _In_ PWSTR Name,
        _Out_opt_ PUNKNOWN *OutUnknownPort,
        _Out_opt_ PUNKNOWN *OutUnknownMiniport
    );

    NTSTATUS CacheSubdevice
    (
        _In_ PWSTR Name,
        _In_ PUNKNOWN UnknownPort,
        _In_ PUNKNOWN UnknownMiniport
    );
    
    NTSTATUS RemoveCachedSubdevice
    (
        _In_ PWSTR Name
    );

    VOID EmptySubdeviceCache();

    NTSTATUS CreateAudioInterfaceWithProperties
    (
        _In_ PCWSTR                                                 ReferenceString,
        _In_ ULONG                                                  cPropertyCount,
        _In_reads_(cPropertyCount) const SYSVAD_DEVPROPERTY        *pProperties,
        _Out_ _At_(AudioSymbolicLinkName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING AudioSymbolicLinkName
    );
};

typedef struct _MINIPAIR_UNKNOWN
{
    LIST_ENTRY              ListEntry;
    WCHAR                   Name[MAX_PATH];
    PUNKNOWN                PortInterface;
    PUNKNOWN                MiniportInterface;
    PADAPTERPOWERMANAGEMENT PowerInterface;
} MINIPAIR_UNKNOWN;

//
// Used to implement the singleton pattern.
//
LONG  CAdapterCommon::m_AdapterInstances = 0;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS SysvadIoSetDeviceInterfacePropertyDataMultiple
(
    _In_ PUNICODE_STRING                                        SymbolicLinkName,
    _In_ ULONG                                                  cPropertyCount,
    _In_reads_(cPropertyCount) const SYSVAD_DEVPROPERTY        *pProperties
)
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    for (ULONG i = 0; i < cPropertyCount; i++)
    {
        ntStatus = IoSetDeviceInterfacePropertyData(
            SymbolicLinkName,
            pProperties[i].PropertyKey,
            LOCALE_NEUTRAL,
            PLUGPLAY_PROPERTY_PERSISTENT,
            pProperties[i].Type,
            pProperties[i].BufferSize,
            pProperties[i].Buffer);

        if (!NT_SUCCESS(ntStatus))
        {
            return ntStatus;
        }
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
NewAdapterCommon
( 
    _Out_       PUNKNOWN *              Unknown,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN                UnknownOuter,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
        __drv_reportError("Must succeed pool allocations are forbidden. "
			    "Allocation failures cause a system crash"))
    _In_        POOL_TYPE               PoolType 
)
/*++

Routine Description:

  Creates a new CAdapterCommon

Arguments:

  Unknown - 

  UnknownOuter -

  PoolType

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Unknown);

    NTSTATUS ntStatus;

    //
    // This sample supports only one instance of this object.
    // (b/c of CSaveData's static members and Bluetooth HFP logic). 
    //
    if (CAdapterCommon::m_AdapterInstances != 0)
    {
        ntStatus = STATUS_DEVICE_BUSY;
        DPF(D_ERROR, "NewAdapterCommon failed, only one instance is allowed");
        goto Done;
    }
    
    CAdapterCommon::m_AdapterInstances++;

    //
    // Allocate an adapter object.
    //
    CAdapterCommon *p = new(PoolType, MINADAPTER_POOLTAG) CAdapterCommon(UnknownOuter);
    if (p == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        DPF(D_ERROR, "NewAdapterCommon failed, 0x%x", ntStatus);
        goto Done;
    }

    // 
    // Success.
    //
    *Unknown = PUNKNOWN((PADAPTERCOMMON)(p));
    (*Unknown)->AddRef(); 
    ntStatus = STATUS_SUCCESS; 

Done:    
    return ntStatus;
} // NewAdapterCommon

//=============================================================================
#pragma code_seg("PAGE")
CAdapterCommon::~CAdapterCommon
( 
    void 
)
/*++

Routine Description:

  Destructor for CAdapterCommon.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::~CAdapterCommon]");

    if (m_pHW)
    {
        delete m_pHW;
        m_pHW = NULL;
    }
    
    CSaveData::DestroyWorkItems();

    SAFE_RELEASE(m_pPortClsEtwHelper);
    SAFE_RELEASE(m_pServiceGroupWave);
 
    if (m_WdfDevice)
    {
        WdfObjectDelete(m_WdfDevice);
        m_WdfDevice = NULL;
    }

    CAdapterCommon::m_AdapterInstances--;
    ASSERT(CAdapterCommon::m_AdapterInstances == 0);
} // ~CAdapterCommon  

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(PDEVICE_OBJECT)   
CAdapterCommon::GetDeviceObject
(
    void
)
/*++

Routine Description:

  Returns the deviceobject

Arguments:

Return Value:

  PDEVICE_OBJECT

--*/
{
    PAGED_CODE();
    
    return m_pDeviceObject;
} // GetDeviceObject

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(PDEVICE_OBJECT)   
CAdapterCommon::GetPhysicalDeviceObject
(
    void
)
/*++

Routine Description:

  Returns the PDO.

Arguments:

Return Value:

  PDEVICE_OBJECT

--*/
{
    PAGED_CODE();
    
    return m_pPhysicalDeviceObject;
} // GetPhysicalDeviceObject

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(WDFDEVICE)   
CAdapterCommon::GetWdfDevice
(
    void
)
/*++

Routine Description:

  Returns the associated WDF miniport device. Note that this is NOT an audio
  miniport. The WDF miniport device is the WDF device associated with the
  adapter.

Arguments:

Return Value:

  WDFDEVICE

--*/
{
    PAGED_CODE();
    
    return m_WdfDevice;
} // GetWdfDevice

#pragma code_seg("PAGE")
PI2S_FUNCTION_INTERFACE CAdapterCommon::GetI2sFunctionInterface(void)
{
	return &m_I2sFunctionInterface;
}

#pragma code_seg("PAGE")
PCODEC_FUNCTION_INTERFACE CAdapterCommon::GetCodecFunctionInterface(void)
{
	return &m_CodecFunctionInterface;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::Init
( 
    _In_  PDEVICE_OBJECT          DeviceObject 
)
/*++

Routine Description:

    Initialize adapter common object.

Arguments:

    DeviceObject - pointer to the device object

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::Init]");

    ASSERT(DeviceObject);

    NTSTATUS        ntStatus    = STATUS_SUCCESS;
    m_pServiceGroupWave     = NULL;
    m_pDeviceObject         = DeviceObject;
    m_pPhysicalDeviceObject = NULL;
    m_WdfDevice             = NULL;
    m_PowerState            = PowerDeviceD0;
    m_pHW                   = NULL;
    m_pPortClsEtwHelper     = NULL;

    InitializeListHead(&m_SubdeviceCache);

    //
    // Get the PDO.
    //
    ntStatus = PcGetPhysicalDeviceObject(DeviceObject, &m_pPhysicalDeviceObject);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, "PcGetPhysicalDeviceObject failed, 0x%x", ntStatus),
        Done);

    //
    // Create a WDF miniport to represent the adapter. Note that WDF miniports 
    // are NOT audio miniports. An audio adapter is associated with a single WDF
    // miniport. This driver uses WDF to simplify the handling of the Bluetooth
    // SCO HFP Bypass interface.
    //
    ntStatus = WdfDeviceMiniportCreate( WdfGetDriver(),
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        DeviceObject,           // FDO
                                        NULL,                   // Next device.
                                        NULL,                   // PDO
                                       &m_WdfDevice);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, "WdfDeviceMiniportCreate failed, 0x%x", ntStatus),
        Done);

    // Initialize HW.
    // 
    m_pHW = new (NonPagedPoolNx, SYSVAD_POOLTAG)  CSYSVADHW;
    if (!m_pHW)
    {
        DPF(D_TERSE, "Insufficient memory for SYSVAD HW");
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_JUMP(ntStatus, Done);
    
    m_pHW->MixerReset();

    //
    // Initialize SaveData class.
    //
    CSaveData::SetDeviceObject(DeviceObject);   //device object is needed by CSaveData
    ntStatus = CSaveData::InitializeWorkItems(DeviceObject);
    IF_FAILED_JUMP(ntStatus, Done);

Done:

    return ntStatus;
} // Init

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CAdapterCommon::MixerReset
( 
    void 
)
/*++

Routine Description:

  Reset mixer registers from registry.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();
    
    if (m_pHW)
    {
        m_pHW->MixerReset();
    }
} // MixerReset

//=============================================================================
/* Here are the definitions of the standard miniport events.

Event type	: eMINIPORT_IHV_DEFINED 
Parameter 1	: Defined and used by IHVs
Parameter 2	: Defined and used by IHVs
Parameter 3	:Defined and used by IHVs
Parameter 4 :Defined and used by IHVs

Event type: eMINIPORT_BUFFER_COMPLETE
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received	
Parameter 3: Data length completed
Parameter 4:0

Event type: eMINIPORT_PIN_STATE
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received		
Parameter 3: Pin State 0->KS_STOP, 1->KS_ACQUIRE, 2->KS_PAUSE, 3->KS_RUN 
Parameter 4:0

Event type: eMINIPORT_GET_STREAM_POS
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received	
Parameter 3: 0
Parameter 4:0


Event type: eMINIPORT_SET_WAVERT_BUFFER_WRITE_POS
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received	
Parameter 3: the arget WaveRtBufferWritePosition received from portcls
Parameter 4:0

Event type: eMINIPORT_GET_PRESENTATION_POS
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received	
Parameter 3: Presentation position
Parameter 4:0

Event type: eMINIPORT_PROGRAM_DMA 
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received	
Parameter 3: Starting  WaveRt buffer offset
Parameter 4: Data length

Event type: eMINIPORT_GLITCH_REPORT
Parameter 1: Current linear buffer position	
Parameter 2: the previous WaveRtBufferWritePosition that the drive received	
Parameter 3: major glitch code: 1:WaveRT buffer is underrun, 
                                2:decoder errors, 
                                3:receive the same wavert buffer two in a row in event driven mode
Parameter 4: minor code for the glitch cause

Event type: eMINIPORT_LAST_BUFFER_RENDERED
Parameter 1: Current linear buffer position	
Parameter 2: the very last WaveRtBufferWritePosition that the driver received	
Parameter 3: 0
Parameter 4: 0

*/
#pragma code_seg()
STDMETHODIMP
CAdapterCommon::WriteEtwEvent
( 
    _In_ EPcMiniportEngineEvent    miniportEventType,
    _In_ ULONGLONG  ullData1,
    _In_ ULONGLONG  ullData2,
    _In_ ULONGLONG  ullData3,
    _In_ ULONGLONG  ullData4
)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (m_pPortClsEtwHelper)
    {
        ntStatus = m_pPortClsEtwHelper->MiniportWriteEtwEvent( miniportEventType, ullData1, ullData2, ullData3, ullData4) ;
    }
    return ntStatus;
} // WriteEtwEvent

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CAdapterCommon::SetEtwHelper
( 
    PPORTCLSETWHELPER _pPortClsEtwHelper
)
{
    PAGED_CODE();
    
    SAFE_RELEASE(m_pPortClsEtwHelper);

    m_pPortClsEtwHelper = _pPortClsEtwHelper;

    if (m_pPortClsEtwHelper)
    {
        m_pPortClsEtwHelper->AddRef();
    }
} // SetEtwHelper

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
CAdapterCommon::NonDelegatingQueryInterface
( 
    _In_ REFIID                      Interface,
    _COM_Outptr_ PVOID *        Object 
)
/*++

Routine Description:

  QueryInterface routine for AdapterCommon

Arguments:

  Interface - 

  Object -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterCommon))
    {
        *Object = PVOID(PADAPTERCOMMON(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(void)
CAdapterCommon::SetWaveServiceGroup
( 
    _In_ PSERVICEGROUP            ServiceGroup 
)
/*++

Routine Description:


Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::SetWaveServiceGroup]");
    
    SAFE_RELEASE(m_pServiceGroupWave);

    m_pServiceGroupWave = ServiceGroup;

    if (m_pServiceGroupWave)
    {
        m_pServiceGroupWave->AddRef();
    }
} // SetWaveServiceGroup

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(BOOL)
CAdapterCommon::bDevSpecificRead()
/*++

Routine Description:

  Fetch Device Specific information.

Arguments:

  N/A

Return Value:

    BOOL - Device Specific info

--*/
{
    if (m_pHW)
    {
        return m_pHW->bGetDevSpecific();
    }

    return FALSE;
} // bDevSpecificRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::bDevSpecificWrite
(
    _In_  BOOL                    bDevSpecific
)
/*++

Routine Description:

  Store the new value in the Device Specific location.

Arguments:

  bDevSpecific - Value to store

Return Value:

  N/A.

--*/
{
    if (m_pHW)
    {
        m_pHW->bSetDevSpecific(bDevSpecific);
    }
} // DevSpecificWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(INT)
CAdapterCommon::iDevSpecificRead()
/*++

Routine Description:

  Fetch Device Specific information.

Arguments:

  N/A

Return Value:

    INT - Device Specific info

--*/
{
    if (m_pHW)
    {
        return m_pHW->iGetDevSpecific();
    }

    return 0;
} // iDevSpecificRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::iDevSpecificWrite
(
    _In_  INT                    iDevSpecific
)
/*++

Routine Description:

  Store the new value in the Device Specific location.

Arguments:

  iDevSpecific - Value to store

Return Value:

  N/A.

--*/
{
    if (m_pHW)
    {
        m_pHW->iSetDevSpecific(iDevSpecific);
    }
} // iDevSpecificWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(UINT)
CAdapterCommon::uiDevSpecificRead()
/*++

Routine Description:

  Fetch Device Specific information.

Arguments:

  N/A

Return Value:

    UINT - Device Specific info

--*/
{
    if (m_pHW)
    {
        return m_pHW->uiGetDevSpecific();
    }

    return 0;
} // uiDevSpecificRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::uiDevSpecificWrite
(
    _In_  UINT                    uiDevSpecific
)
/*++

Routine Description:

  Store the new value in the Device Specific location.

Arguments:

  uiDevSpecific - Value to store

Return Value:

  N/A.

--*/
{
    if (m_pHW)
    {
        m_pHW->uiSetDevSpecific(uiDevSpecific);
    }
} // uiDevSpecificWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(BOOL)
CAdapterCommon::MixerMuteRead
(
    _In_  ULONG               Index,
    _In_  ULONG               Channel
)
/*++

Routine Description:

  Store the new value in mixer register array.

Arguments:

  Index - node id

Return Value:

    BOOL - mixer mute setting for this node

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerMute(Index, Channel);
    }

    return 0;
} // MixerMuteRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::MixerMuteWrite
(
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel,
    _In_  BOOL                    Value
)
/*++

Routine Description:

  Store the new value in mixer register array.

Arguments:

  Index - node id

  Value - new mute settings

Return Value:

  NT status code.

--*/
{
    if (m_pHW)
    {
        m_pHW->SetMixerMute(Index, Channel, Value);
    }
} // MixerMuteWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(ULONG)
CAdapterCommon::MixerMuxRead() 
/*++

Routine Description:

  Return the mux selection

Arguments:

  Index - node id

  Value - new mute settings

Return Value:

  NT status code.

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerMux();
    }

    return 0;
} // MixerMuxRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::MixerMuxWrite
(
    _In_  ULONG                   Index
)
/*++

Routine Description:

  Store the new mux selection

Arguments:

  Index - node id

  Value - new mute settings

Return Value:

  NT status code.

--*/
{
    if (m_pHW)
    {
        m_pHW->SetMixerMux(Index);
    }
} // MixerMuxWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(LONG)
CAdapterCommon::MixerVolumeRead
( 
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel
)
/*++

Routine Description:

  Return the value in mixer register array.

Arguments:

  Index - node id

  Channel = which channel

Return Value:

    Byte - mixer volume settings for this line

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerVolume(Index, Channel);
    }

    return 0;
} // MixerVolumeRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::MixerVolumeWrite
( 
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel,
    _In_  LONG                    Value
)
/*++

Routine Description:

  Store the new value in mixer register array.

Arguments:

  Index - node id

  Channel - which channel

  Value - new volume level

Return Value:

    void

--*/
{
    if (m_pHW)
    {
        m_pHW->SetMixerVolume(Index, Channel, Value);
    }
} // MixerVolumeWrite

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(LONG)
CAdapterCommon::MixerPeakMeterRead
( 
    _In_  ULONG                   Index,
    _In_  ULONG                   Channel
)
/*++

Routine Description:

  Return the value in mixer register array.

Arguments:

  Index - node id

  Channel = which channel

Return Value:

    Byte - mixer sample peak meter settings for this line

--*/
{
    if (m_pHW)
    {
        return m_pHW->GetMixerPeakMeter(Index, Channel);
    }

    return 0;
} // MixerVolumeRead

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(void)
CAdapterCommon::PowerChangeState
( 
    _In_  POWER_STATE             NewState 
)
/*++

Routine Description:


Arguments:

  NewState - The requested, new power state for the device. 

Return Value:

    void

Note:
  From MSDN:

  To assist the driver, PortCls will pause any active audio streams prior to calling
  this method to place the device in a sleep state. After calling this method, PortCls
  will unpause active audio streams, to wake the device up. Miniports can opt for 
  additional notification by utilizing the IPowerNotify interface.

  The miniport driver must perform the requested change to the device's power state 
  before it returns from the PowerChangeState call. If the miniport driver needs to 
  save or restore any device state before a power-state change, the miniport driver 
  should support the IPowerNotify interface, which allows it to receive advance warning
  of any such change. Before returning from a successful PowerChangeState call, the 
  miniport driver should cache the new power state.

  While the miniport driver is in one of the sleep states (any state other than 
  PowerDeviceD0), it must avoid writing to the hardware. The miniport driver must cache
  any hardware accesses that need to be deferred until the device powers up again. If
  the power state is changing from one of the sleep states to PowerDeviceD0, the 
  miniport driver should perform any deferred hardware accesses after it has powered up
  the device. If the power state is changing from PowerDeviceD0 to a sleep state, the 
  miniport driver can perform any necessary hardware accesses during the PowerChangeState
  call before it powers down the device.

  While powered down, a miniport driver is never asked to create a miniport driver object
  or stream object. PortCls always places the device in the PowerDeviceD0 state before
  calling the miniport driver's NewStream method.
  
--*/
{
    DPF_ENTER("[CAdapterCommon::PowerChangeState]");

    // Notify all registered miniports of a power state change
    PLIST_ENTRY le = NULL;
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (pRecord->PowerInterface)
        {
            pRecord->PowerInterface->PowerChangeState(NewState);
        }
    }

    // is this actually a state change??
    //
    if (NewState.DeviceState != m_PowerState)
    {
        // switch on new state
        //
        switch (NewState.DeviceState)
        {
            case PowerDeviceD0:
            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                m_PowerState = NewState.DeviceState;

                DPF
                ( 
                    D_VERBOSE, 
                    "Entering D%u", ULONG(m_PowerState) - ULONG(PowerDeviceD0)
                );

                break;
    
            default:
            
                DPF(D_VERBOSE, "Unknown Device Power State");
                break;
        }
    }
} // PowerStateChange

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::QueryDeviceCapabilities
( 
    _Inout_updates_bytes_(sizeof(DEVICE_CAPABILITIES)) PDEVICE_CAPABILITIES    PowerDeviceCaps 
)
/*++

Routine Description:

    Called at startup to get the caps for the device.  This structure provides 
    the system with the mappings between system power state and device power 
    state.  This typically will not need modification by the driver.         

Arguments:

  PowerDeviceCaps - The device's capabilities. 

Return Value:

  NT status code.

--*/
{
    UNREFERENCED_PARAMETER(PowerDeviceCaps);

    DPF_ENTER("[CAdapterCommon::QueryDeviceCapabilities]");

    return (STATUS_SUCCESS);
} // QueryDeviceCapabilities

//=============================================================================
#pragma code_seg()
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::QueryPowerChangeState
( 
    _In_  POWER_STATE             NewStateQuery 
)
/*++

Routine Description:

  Query to see if the device can change to this power state 

Arguments:

  NewStateQuery - The requested, new power state for the device

Return Value:

  NT status code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    DPF_ENTER("[CAdapterCommon::QueryPowerChangeState]");

    // query each miniport for it's power state, we're finished if even one indicates
    // it cannot go to this power state.
    PLIST_ENTRY le = NULL;
    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && NT_SUCCESS(status); le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (pRecord->PowerInterface)
        {
            status = pRecord->PowerInterface->QueryPowerChangeState(NewStateQuery);
        }
    }

    return status;
} // QueryPowerChangeState

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::CreateAudioInterfaceWithProperties
(
    _In_ PCWSTR ReferenceString,
    _In_ ULONG cPropertyCount,
    _In_reads_(cPropertyCount) const SYSVAD_DEVPROPERTY *pProperties,
    _Out_ _At_(AudioSymbolicLinkName->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING AudioSymbolicLinkName
)
/*++

Routine Description:

Create the audio interface (in disabled mode).

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::CreateAudioInterfaceWithProperties]");

    NTSTATUS        ntStatus;
    UNICODE_STRING  referenceString;

    RtlInitUnicodeString(&referenceString, ReferenceString);

    //
    // Reset output value.
    //
    RtlZeroMemory(AudioSymbolicLinkName, sizeof(UNICODE_STRING));

    //
    // Register an audio interface if not already present.
    //
    ntStatus = IoRegisterDeviceInterface(
        GetPhysicalDeviceObject(),
        &KSCATEGORY_AUDIO,
        &referenceString,
        AudioSymbolicLinkName);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, "CreateAudioInterfaceWithProperties: IoRegisterDeviceInterface(KSCATEGORY_AUDIO): failed, 0x%x", ntStatus),
        Done);

    //
    // Set properties on the interface
    //
    ntStatus = SysvadIoSetDeviceInterfacePropertyDataMultiple(AudioSymbolicLinkName, cPropertyCount, pProperties);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, "CreateAudioInterfaceWithProperties: SysvadIoSetDeviceInterfacePropertyDataMultiple(...): failed, 0x%x", ntStatus),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        RtlFreeUnicodeString(AudioSymbolicLinkName);
        RtlZeroMemory(AudioSymbolicLinkName, sizeof(UNICODE_STRING));
    }
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::InstallSubdevice
( 
    _In_opt_        PIRP                                    Irp,
    _In_            PWSTR                                   Name,
    _In_            REFGUID                                 PortClassId,
    _In_            REFGUID                                 MiniportClassId,
    _In_opt_        PFNCREATEMINIPORT                       MiniportCreate,
    _In_            ULONG                                   cPropertyCount,
    _In_reads_opt_(cPropertyCount) const SYSVAD_DEVPROPERTY * pProperties,
    _In_opt_        PVOID                                   DeviceContext,
    _In_            PENDPOINT_MINIPAIR                      MiniportPair,
    _In_opt_        PRESOURCELIST                           ResourceList,
    _In_            REFGUID                                 PortInterfaceId,
    _Out_opt_       PUNKNOWN                              * OutPortInterface,
    _Out_opt_       PUNKNOWN                              * OutPortUnknown,
    _Out_opt_       PUNKNOWN                              * OutMiniportUnknown
)
{
/*++

Routine Description:

    This function creates and registers a subdevice consisting of a port       
    driver, a minport driver and a set of resources bound together.  It will   
    also optionally place a pointer to an interface on the port driver in a    
    specified location before initializing the port driver.  This is done so   
    that a common ISR can have access to the port driver during 
    initialization, when the ISR might fire.                                   

Arguments:

    Irp - pointer to the irp object.

    Name - name of the miniport. Passes to PcRegisterSubDevice
 
    PortClassId - port class id. Passed to PcNewPort.

    MiniportClassId - miniport class id. Passed to PcNewMiniport.

    MiniportCreate - pointer to a miniport creation function. If NULL, 
                     PcNewMiniport is used.
                     
    DeviceContext - deviceType specific.

    MiniportPair - endpoint configuration info.    

    ResourceList - pointer to the resource list.

    PortInterfaceId - GUID that represents the port interface.
       
    OutPortInterface - pointer to store the port interface

    OutPortUnknown - pointer to store the unknown port interface.

    OutMiniportUnknown - pointer to store the unknown miniport interface

Return Value:

    NT status code.

--*/
    PAGED_CODE();
    DPF_ENTER("[InstallSubDevice %S]", Name);

    ASSERT(Name != NULL);
    ASSERT(m_pDeviceObject != NULL);

    NTSTATUS                    ntStatus;
    PPORT                       port            = NULL;
    PUNKNOWN                    miniport        = NULL;
    PADAPTERCOMMON              adapterCommon   = NULL;
    UNICODE_STRING              symbolicLink    = { 0 };

    adapterCommon = PADAPTERCOMMON(this);

    ntStatus = CreateAudioInterfaceWithProperties(Name, cPropertyCount, pProperties, &symbolicLink);
    if (NT_SUCCESS(ntStatus))
    {
        // Currently have no use for the symbolic link
        RtlFreeUnicodeString(&symbolicLink);

        // Create the port driver object
        //
        ntStatus = PcNewPort(&port, PortClassId);
    }

    // Create the miniport object
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (MiniportCreate)
        {
            ntStatus = 
                MiniportCreate
                ( 
                    &miniport,
                    MiniportClassId,
                    NULL,
                    NonPagedPoolNx,
                    adapterCommon,
                    DeviceContext,
                    MiniportPair
                );
        }
        else
        {
            ntStatus = 
                PcNewMiniport
                (
                    (PMINIPORT *) &miniport, 
                    MiniportClassId
                );
        }
    }

    // Init the port driver and miniport in one go.
    //
    if (NT_SUCCESS(ntStatus))
    {
#pragma warning(push)
        // IPort::Init's annotation on ResourceList requires it to be non-NULL.  However,
        // for dynamic devices, we may no longer have the resource list and this should
        // still succeed.
        //
#pragma warning(disable:6387)
        ntStatus = 
            port->Init
            ( 
                m_pDeviceObject,
                Irp,
                miniport,
                adapterCommon,
                ResourceList 
            );
#pragma warning (pop)

        if (NT_SUCCESS(ntStatus))
        {
            // Register the subdevice (port/miniport combination).
            //
            ntStatus = 
                PcRegisterSubdevice
                ( 
                    m_pDeviceObject,
                    Name,
                    port 
                );
        }
    }

    // Deposit the port interfaces if it's needed.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (OutPortUnknown)
        {
            ntStatus = 
                port->QueryInterface
                ( 
                    IID_IUnknown,
                    (PVOID *)OutPortUnknown 
                );
        }

        if (OutPortInterface)
        {
            ntStatus = 
                port->QueryInterface
                ( 
                    PortInterfaceId,
                    (PVOID *) OutPortInterface 
                );
        }

        if (OutMiniportUnknown)
        {
            ntStatus = 
                miniport->QueryInterface
                ( 
                    IID_IUnknown,
                    (PVOID *)OutMiniportUnknown 
                );
        }

    }

    if (port)
    {
        port->Release();
    }

    if (miniport)
    {
        miniport->Release();
    }

    return ntStatus;
} // InstallSubDevice

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::UnregisterSubdevice
(
    _In_opt_   PUNKNOWN     UnknownPort
)
/*++

Routine Description:

  Unregisters and releases the specified subdevice.

Arguments:

  UnknownPort - Wave or topology port interface.

Return Value:

  NTSTATUS

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::UnregisterSubdevice]");

    ASSERT(m_pDeviceObject != NULL);
    
    NTSTATUS                ntStatus            = STATUS_SUCCESS;
    PUNREGISTERSUBDEVICE    unregisterSubdevice = NULL;
    
    if (NULL == UnknownPort)
    {
        return ntStatus;
    }

    //
    // Get the IUnregisterSubdevice interface.
    //
    ntStatus = UnknownPort->QueryInterface( 
        IID_IUnregisterSubdevice,
        (PVOID *)&unregisterSubdevice);

    //
    // Unregister the port object.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = unregisterSubdevice->UnregisterSubdevice(
            m_pDeviceObject,
            UnknownPort);

        //
        // Release the IUnregisterSubdevice interface.
        //
        unregisterSubdevice->Release();
    }
    
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::ConnectTopologies
(
    _In_ PUNKNOWN                   UnknownTopology,
    _In_ PUNKNOWN                   UnknownWave,
    _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
    _In_ ULONG                      PhysicalConnectionCount
)
/*++

Routine Description:

  Connects the bridge pins between the wave and mixer topologies.

Arguments:

Return Value:

  NTSTATUS

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::ConnectTopologies]");
    
    ASSERT(m_pDeviceObject != NULL);
    
    NTSTATUS        ntStatus            = STATUS_SUCCESS;

    //
    // register wave <=> topology connections
    // This will connect bridge pins of wave and topology
    // miniports.
    //
    for (ULONG i = 0; i < PhysicalConnectionCount && NT_SUCCESS(ntStatus); i++)
    {
    
        switch(PhysicalConnections[i].eType)
        {
            case CONNECTIONTYPE_TOPOLOGY_OUTPUT:
                ntStatus =
                    PcRegisterPhysicalConnection
                    ( 
                        m_pDeviceObject,
                        UnknownTopology,
                        PhysicalConnections[i].ulTopology,
                        UnknownWave,
                        PhysicalConnections[i].ulWave
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_TERSE, "ConnectTopologies: PcRegisterPhysicalConnection(render) failed, 0x%x", ntStatus);
                }
                break;
            case CONNECTIONTYPE_WAVE_OUTPUT:
                ntStatus =
                    PcRegisterPhysicalConnection
                    ( 
                        m_pDeviceObject,
                        UnknownWave,
                        PhysicalConnections[i].ulWave,
                        UnknownTopology,
                        PhysicalConnections[i].ulTopology
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_TERSE, "ConnectTopologies: PcRegisterPhysicalConnection(capture) failed, 0x%x", ntStatus);
                }
                break;
        }
    }    

    //
    // Cleanup in case of error.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        // disconnect all connections on error, ignore error code because not all
        // connections may have been made
        DisconnectTopologies(UnknownTopology, UnknownWave, PhysicalConnections, PhysicalConnectionCount);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::DisconnectTopologies
(
    _In_ PUNKNOWN                   UnknownTopology,
    _In_ PUNKNOWN                   UnknownWave,
    _In_ PHYSICALCONNECTIONTABLE*   PhysicalConnections,
    _In_ ULONG                      PhysicalConnectionCount
)
/*++

Routine Description:

  Disconnects the bridge pins between the wave and mixer topologies.

Arguments:

Return Value:

  NTSTATUS

--*/
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::DisconnectTopologies]");
    
    ASSERT(m_pDeviceObject != NULL);
    
    NTSTATUS                        ntStatus                        = STATUS_SUCCESS;
    NTSTATUS                        ntStatus2                       = STATUS_SUCCESS;
    PUNREGISTERPHYSICALCONNECTION   unregisterPhysicalConnection    = NULL;

    //
    // Get the IUnregisterPhysicalConnection interface
    //
    ntStatus = UnknownTopology->QueryInterface( 
        IID_IUnregisterPhysicalConnection,
        (PVOID *)&unregisterPhysicalConnection);
    
    if (NT_SUCCESS(ntStatus))
    { 
        for (ULONG i = 0; i < PhysicalConnectionCount; i++)
        {
            switch(PhysicalConnections[i].eType)
            {
                case CONNECTIONTYPE_TOPOLOGY_OUTPUT:
                    ntStatus =
                        unregisterPhysicalConnection->UnregisterPhysicalConnection(
                            m_pDeviceObject,
                            UnknownTopology,
                            PhysicalConnections[i].ulTopology,
                            UnknownWave,
                            PhysicalConnections[i].ulWave
                        );

                    if (!NT_SUCCESS(ntStatus))
                    {
                        DPF(D_TERSE, "DisconnectTopologies: UnregisterPhysicalConnection(render) failed, 0x%x", ntStatus);
                    }
                    break;
                case CONNECTIONTYPE_WAVE_OUTPUT:
                    ntStatus =
                        unregisterPhysicalConnection->UnregisterPhysicalConnection(
                            m_pDeviceObject,
                            UnknownWave,
                            PhysicalConnections[i].ulWave,
                            UnknownTopology,
                            PhysicalConnections[i].ulTopology
                        );
                    if (!NT_SUCCESS(ntStatus2))
                    {
                        DPF(D_TERSE, "DisconnectTopologies: UnregisterPhysicalConnection(capture) failed, 0x%x", ntStatus2);
                    }
                    break;
            }

            // cache and return the first error encountered, as it's likely the most relevent
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = ntStatus2;
            }
        }    
    }
    
    //
    // Release the IUnregisterPhysicalConnection interface.
    //
    SAFE_RELEASE(unregisterPhysicalConnection);

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::GetCachedSubdevice
(
    _In_ PWSTR Name,
    _Out_opt_ PUNKNOWN *OutUnknownPort,
    _Out_opt_ PUNKNOWN *OutUnknownMiniport
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::GetCachedSubdevice]");

    // search list, return interface to device if found, fail if not found
    PLIST_ENTRY le = NULL;
    BOOL bFound = FALSE;

    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bFound; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (0 == wcscmp(Name, pRecord->Name))
        {
            if (OutUnknownPort)
            {
                *OutUnknownPort = pRecord->PortInterface;
                (*OutUnknownPort)->AddRef();
            }

            if (OutUnknownMiniport)
            {
                *OutUnknownMiniport = pRecord->MiniportInterface;
                (*OutUnknownMiniport)->AddRef();
            }

            bFound = TRUE;
        }
    }

    return bFound?STATUS_SUCCESS:STATUS_OBJECT_NAME_NOT_FOUND;
}



//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::CacheSubdevice
(
    _In_ PWSTR Name,
    _In_ PUNKNOWN UnknownPort,
    _In_ PUNKNOWN UnknownMiniport
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::CacheSubdevice]");

    // add the item with this name/interface to the list
    NTSTATUS         ntStatus       = STATUS_SUCCESS;
    MINIPAIR_UNKNOWN *pNewSubdevice = NULL;

    pNewSubdevice = new(NonPagedPoolNx, MINADAPTER_POOLTAG) MINIPAIR_UNKNOWN;

    if (!pNewSubdevice)
    {
        DPF(D_TERSE, "Insufficient memory to cache subdevice");
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus))
    {
        memset(pNewSubdevice, 0, sizeof(MINIPAIR_UNKNOWN));

        ntStatus = RtlStringCchCopyW(pNewSubdevice->Name, SIZEOF_ARRAY(pNewSubdevice->Name), Name);
    }

    if (NT_SUCCESS(ntStatus))
    {
        pNewSubdevice->PortInterface = UnknownPort;
        pNewSubdevice->PortInterface->AddRef();

        pNewSubdevice->MiniportInterface = UnknownMiniport;
        pNewSubdevice->MiniportInterface->AddRef();

        // cache the IAdapterPowerManagement interface (if available) from the filter. Some endpoints,
        // like FM and cellular, have their own power requirements that we must track. If this fails,
        // it just means this filter doesn't do power management.
        UnknownMiniport->QueryInterface(IID_IAdapterPowerManagement, (PVOID *)&(pNewSubdevice->PowerInterface));

        InsertTailList(&m_SubdeviceCache, &pNewSubdevice->ListEntry);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (pNewSubdevice)
        {
            delete pNewSubdevice;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CAdapterCommon::RemoveCachedSubdevice
(
    _In_ PWSTR Name
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::RemoveCachedSubdevice]");

    // search list, remove the entry from the list

    PLIST_ENTRY le = NULL;
    BOOL bRemoved = FALSE;

    for (le = m_SubdeviceCache.Flink; le != &m_SubdeviceCache && !bRemoved; le = le->Flink)
    {
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        if (0 == wcscmp(Name, pRecord->Name))
        {
            SAFE_RELEASE(pRecord->PortInterface);
            SAFE_RELEASE(pRecord->MiniportInterface);
            SAFE_RELEASE(pRecord->PowerInterface);
            memset(pRecord->Name, 0, sizeof(pRecord->Name));
            RemoveEntryList(le);
            bRemoved = TRUE;
            delete pRecord;
            break;
        }
    }

    return bRemoved?STATUS_SUCCESS:STATUS_OBJECT_NAME_NOT_FOUND;
}

#pragma code_seg("PAGE")
VOID
CAdapterCommon::EmptySubdeviceCache()
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::EmptySubdeviceCache]");

    while (!IsListEmpty(&m_SubdeviceCache))
    {
        PLIST_ENTRY le = RemoveHeadList(&m_SubdeviceCache);
        MINIPAIR_UNKNOWN *pRecord = CONTAINING_RECORD(le, MINIPAIR_UNKNOWN, ListEntry);

        SAFE_RELEASE(pRecord->PortInterface);
        SAFE_RELEASE(pRecord->MiniportInterface);
        SAFE_RELEASE(pRecord->PowerInterface);
        memset(pRecord->Name, 0, sizeof(pRecord->Name));

        delete pRecord;
    }
}

#pragma code_seg("PAGE")
VOID
CAdapterCommon::Cleanup()
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::Cleanup]");

    EmptySubdeviceCache();
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::InstallEndpointFilters
(
    _In_opt_    PIRP                Irp, 
    _In_        PENDPOINT_MINIPAIR  MiniportPair,
    _In_opt_    PVOID               DeviceContext,
    _Out_opt_   PUNKNOWN *          UnknownTopology,
    _Out_opt_   PUNKNOWN *          UnknownWave,
    _Out_opt_   PUNKNOWN *          UnknownMiniportTopology,
    _Out_opt_   PUNKNOWN *          UnknownMiniportWave
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::InstallEndpointFilters]");
    
    NTSTATUS            ntStatus            = STATUS_SUCCESS;
    PUNKNOWN            unknownTopology     = NULL;
    PUNKNOWN            unknownWave         = NULL;
    BOOL                bTopologyCreated    = FALSE;
    BOOL                bWaveCreated        = FALSE;
    PUNKNOWN            unknownMiniTopo     = NULL;
    PUNKNOWN            unknownMiniWave     = NULL;

    // Initialize output optional parameters if needed
    if (UnknownTopology)
    {
        *UnknownTopology = NULL;
    }

    if (UnknownWave)
    {
        *UnknownWave = NULL;
    }
  
    if (UnknownMiniportTopology)
    {
        *UnknownMiniportTopology = NULL;
    }

    if (UnknownMiniportWave)
    {
        *UnknownMiniportWave = NULL;
    }

    ntStatus = GetCachedSubdevice(MiniportPair->TopoName, &unknownTopology, &unknownMiniTopo);
    if (!NT_SUCCESS(ntStatus) || NULL == unknownTopology || NULL == unknownMiniTopo)
    {
        bTopologyCreated = TRUE;

        // Install SYSVAD topology miniport for the render endpoint.
        //
        ntStatus = InstallSubdevice(Irp,
                                    MiniportPair->TopoName, // make sure this name matches with SYSVAD.<TopoName>.szPname in the inf's [Strings] section
                                    CLSID_PortTopology,
                                    CLSID_PortTopology, 
                                    MiniportPair->TopoCreateCallback,
                                    MiniportPair->TopoInterfacePropertyCount,
                                    MiniportPair->TopoInterfaceProperties,
                                    DeviceContext,
                                    MiniportPair,
                                    NULL,
                                    IID_IPortTopology,
                                    NULL,
                                    &unknownTopology,
                                    &unknownMiniTopo
                                    );
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = CacheSubdevice(MiniportPair->TopoName, unknownTopology, unknownMiniTopo);
        }
    }

    ntStatus = GetCachedSubdevice(MiniportPair->WaveName, &unknownWave, &unknownMiniWave);
    if (!NT_SUCCESS(ntStatus) || NULL == unknownWave || NULL == unknownMiniWave)
    {
        bWaveCreated = TRUE;

        // Install SYSVAD wave miniport for the render endpoint.
        //
        ntStatus = InstallSubdevice(Irp,
                                    MiniportPair->WaveName, // make sure this name matches with SYSVAD.<WaveName>.szPname in the inf's [Strings] section
                                    CLSID_PortWaveRT,
                                    CLSID_PortWaveRT,   
                                    MiniportPair->WaveCreateCallback,
                                    MiniportPair->WaveInterfacePropertyCount,
                                    MiniportPair->WaveInterfaceProperties,
                                    DeviceContext,
                                    MiniportPair,
                                    NULL,
                                    IID_IPortWaveRT,
                                    NULL, 
                                    &unknownWave,
                                    &unknownMiniWave
                                    );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = CacheSubdevice(MiniportPair->WaveName, unknownWave, unknownMiniWave);
        }
    }

    if (unknownTopology && unknownWave)
    {
        //
        // register wave <=> topology connections
        // This will connect bridge pins of wave and topology
        // miniports.
        //
        ntStatus = ConnectTopologies(
            unknownTopology,
            unknownWave,
            MiniportPair->PhysicalConnections,
            MiniportPair->PhysicalConnectionCount);
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Set output parameters.
        //
        if (UnknownTopology != NULL && unknownTopology != NULL)
        {
            unknownTopology->AddRef();
            *UnknownTopology = unknownTopology;
        }
        
        if (UnknownWave != NULL && unknownWave != NULL)
        {
            unknownWave->AddRef();
            *UnknownWave = unknownWave;
        }
        if (UnknownMiniportTopology != NULL && unknownMiniTopo != NULL)
        {
            unknownMiniTopo->AddRef();
            *UnknownMiniportTopology = unknownMiniTopo;
        }

        if (UnknownMiniportWave != NULL && unknownMiniWave != NULL)
        {
            unknownMiniWave->AddRef();
            *UnknownMiniportWave = unknownMiniWave;
        }

    }
    else
    {
        if (bTopologyCreated && unknownTopology != NULL)
        {
            UnregisterSubdevice(unknownTopology);
            RemoveCachedSubdevice(MiniportPair->TopoName);
        }
            
        if (bWaveCreated && unknownWave != NULL)
        {
            UnregisterSubdevice(unknownWave);
            RemoveCachedSubdevice(MiniportPair->WaveName);
        }
    }
   
    SAFE_RELEASE(unknownMiniTopo);
    SAFE_RELEASE(unknownTopology);
    SAFE_RELEASE(unknownMiniWave);
    SAFE_RELEASE(unknownWave);

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::RemoveEndpointFilters
(
    _In_        PENDPOINT_MINIPAIR  MiniportPair,
    _In_opt_    PUNKNOWN            UnknownTopology,
    _In_opt_    PUNKNOWN            UnknownWave
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::RemoveEndpointFilters]");
    
    NTSTATUS    ntStatus   = STATUS_SUCCESS;
    
    if (UnknownTopology != NULL && UnknownWave != NULL)
    {
        ntStatus = DisconnectTopologies(
            UnknownTopology,
            UnknownWave,
            MiniportPair->PhysicalConnections,
            MiniportPair->PhysicalConnectionCount);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_VERBOSE, "RemoveEndpointFilters: DisconnectTopologies failed: 0x%x", ntStatus);
        }
    }

        
    RemoveCachedSubdevice(MiniportPair->WaveName);

    ntStatus = UnregisterSubdevice(UnknownWave);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_VERBOSE, "RemoveEndpointFilters: UnregisterSubdevice(wave) failed: 0x%x", ntStatus);
    }

    RemoveCachedSubdevice(MiniportPair->TopoName);

    ntStatus = UnregisterSubdevice(UnknownTopology);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_VERBOSE, "RemoveEndpointFilters: UnregisterSubdevice(topology) failed: 0x%x", ntStatus);
    }

    //
    // All Done.
    //
    ntStatus = STATUS_SUCCESS;
    
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::GetFilters
(
    _In_        PENDPOINT_MINIPAIR  MiniportPair,
    _Out_opt_   PUNKNOWN *          UnknownTopologyPort,
    _Out_opt_   PUNKNOWN *          UnknownTopologyMiniport,
    _Out_opt_   PUNKNOWN *          UnknownWavePort,
    _Out_opt_   PUNKNOWN *          UnknownWaveMiniport
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::GetFilters]");
    
    NTSTATUS    ntStatus   = STATUS_SUCCESS; 
    PUNKNOWN            unknownTopologyPort     = NULL;
    PUNKNOWN            unknownTopologyMiniport = NULL;
    PUNKNOWN            unknownWavePort         = NULL;
    PUNKNOWN            unknownWaveMiniport     = NULL;

    // if the client requested the topology filter, find it and return it
    if (UnknownTopologyPort != NULL || UnknownTopologyMiniport != NULL)
    {
        ntStatus = GetCachedSubdevice(MiniportPair->TopoName, &unknownTopologyPort, &unknownTopologyMiniport);
        if (NT_SUCCESS(ntStatus))
        {
            if (UnknownTopologyPort)
            {
                *UnknownTopologyPort = unknownTopologyPort;
            }

            if (UnknownTopologyMiniport)
            {
                *UnknownTopologyMiniport = unknownTopologyMiniport;
            }
        }
    }

    // if the client requested the wave filter, find it and return it
    if (NT_SUCCESS(ntStatus) && (UnknownWavePort != NULL || UnknownWaveMiniport != NULL))
    {
        ntStatus = GetCachedSubdevice(MiniportPair->WaveName, &unknownWavePort, &unknownWaveMiniport);
        if (NT_SUCCESS(ntStatus))
        {
            if (UnknownWavePort)
            {
                *UnknownWavePort = unknownWavePort;
            }

            if (UnknownWaveMiniport)
            {
                *UnknownWaveMiniport = unknownWaveMiniport;
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::SetIdlePowerManagement
(
  _In_  PENDPOINT_MINIPAIR  MiniportPair,
  _In_  BOOL bEnabled
)
{
    PAGED_CODE();
    DPF_ENTER("[CAdapterCommon::SetIdlePowerManagement]");

    NTSTATUS      ntStatus   = STATUS_SUCCESS; 
    IUnknown      *pUnknown = NULL;
    PPORTCLSPOWER pPortClsPower = NULL;
    // refcounting disable requests. Each miniport is responsible for calling this in pairs,
    // disable on the first request to disable, enable on the last request to enable.

    // make sure that we always call SetIdlePowerManagment using the IPortClsPower
    // from the requesting port, so we don't cache a reference to a port
    // indefinitely, preventing it from ever unloading.
    ntStatus = GetFilters(MiniportPair, NULL, NULL, &pUnknown, NULL);
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = 
            pUnknown->QueryInterface
            (
                IID_IPortClsPower,
                (PVOID*) &pPortClsPower
            );
    }

    if (NT_SUCCESS(ntStatus))
    {
        if (bEnabled)
        {
            m_dwIdleRequests--;

            if (0 == m_dwIdleRequests)
            {
                pPortClsPower->SetIdlePowerManagement(m_pDeviceObject, TRUE);
            }
        }
        else
        {
            if (0 == m_dwIdleRequests)
            {
                pPortClsPower->SetIdlePowerManagement(m_pDeviceObject, FALSE);
            }

            m_dwIdleRequests++;
        }
    }

    SAFE_RELEASE(pUnknown);
    SAFE_RELEASE(pPortClsPower);

    return ntStatus;
}

NTSTATUS CAdapterCommon::SetPowerDownCompletionCallback()
{
	NTSTATUS Status = STATUS_SUCCESS;
	I2S_POWER_DOWN_COMPLETION_CALLBACK I2SPowerDownCompletionCallback = { 0 };
	CODEC_POWER_DOWN_COMPLETION_CALLBACK CodecPowerDownCompletionCallback = { 0 };

	DPF_ENTER("[SetPowerDownCompletionCallback]");

	I2SPowerDownCompletionCallback.pCallbackContext = this;
	I2SPowerDownCompletionCallback.pCallbackRoutine = (PI2SPowerDownCompletionCallbackRoutine)I2SPowerDownCompletionCallbackRoutine;

	Status = m_I2sFunctionInterface.I2SSetPowerDownCompletionCallback(
		m_I2sFunctionInterface.InterfaceHeader.Context, &I2SPowerDownCompletionCallback);
	if (!NT_SUCCESS(Status))
	{
		DPF(D_ERROR, "Failed to set power up completion callback to I2S with 0x%lx.", Status);
		goto Exit;
	}

	CodecPowerDownCompletionCallback.pCallbackContext = this;
	CodecPowerDownCompletionCallback.pCallbackRoutine = (PCodecPowerDownCompletionCallbackRoutine)CodecPowerDownCompletionCallbackRoutine;
	Status = m_CodecFunctionInterface.CodecSetPowerDownCompletionCallback(
		m_CodecFunctionInterface.InterfaceHeader.Context, &CodecPowerDownCompletionCallback);
	if (!NT_SUCCESS(Status))
	{
		DPF(D_ERROR, "Failed to set power up completion callback to codec with 0x%lx.", Status);
		goto Exit;
	}

Exit:
	DPF_ENTER("Exit with 0x%lx.", Status);
	return Status;
}

NTSTATUS CAdapterCommon::SetPowerState
(
	_In_ DEVICE_POWER_STATE PowerState
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	LARGE_INTEGER D3ReadyTimeout = { 0 };

	DPF_ENTER("[SetPowerState]");

	m_PowerState = PowerState;
	//DbgPrint_E("Entering PowerDeviceD%u.", (UINT)PowerState - (UINT)PowerDeviceD0);

	Status = m_CodecFunctionInterface.CodecSetPowerState(m_CodecFunctionInterface.InterfaceHeader.Context, PowerState);
	if (!NT_SUCCESS(Status))
	{
		DPF(D_ERROR, "Failed to set power state change to codec with 0x%lx.", Status);
		goto Exit;
	}

	Status = m_I2sFunctionInterface.I2SSetPowerState(m_I2sFunctionInterface.InterfaceHeader.Context, PowerState);
	if (!NT_SUCCESS(Status))
	{
		DPF(D_ERROR, "Failed to set power state change to I2S with 0x%lx.", Status);
		goto Exit;
	}

	if (PowerDeviceD3 == PowerState)
	{
		D3ReadyTimeout.QuadPart = WDF_REL_TIMEOUT_IN_MS(DEVICE_D3_READY_WAIT_TIMEOUT_MS);
		Status = KeWaitForMultipleObjects(DEVICE_D3_READY_EVENT_COUNT,
			(PVOID *)m_pDeviceD3ReadyEvent,
			WaitAll,
			Executive,
			KernelMode,
			FALSE,
			&D3ReadyTimeout,
			NULL);

		// TODO: maybe the timeout setting will be changed after the compatible test
		if (STATUS_TIMEOUT == Status)
		{
			DPF(D_ERROR, "Timeout of the I2S/code device D3 ready waiting.");
			goto Exit;
		}
	}

Exit:
	DPF_ENTER("Exit with 0x%lx.", Status);
	return Status;

}
#pragma code_seg("PAGE")
NTSTATUS I2SPowerDownCompletionCallbackRoutine
(
	_In_ PVOID pContext
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	CAdapterCommon *pAdapterCommon = NULL;

	DPF_ENTER("[I2SPowerDownCompletionCallbackRoutine]");

	if (NULL == pContext)
	{
		DPF(D_ERROR, "Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pAdapterCommon = (CAdapterCommon *)pContext;

	//KeSetEvent(&pAdapterCommon->m_I2SD3ReadyEvent, 0, FALSE);

Exit:
	DPF_ENTER("Exit with 0x%lx.", Status);
	return Status;
}

#pragma code_seg("PAGE")
NTSTATUS CodecPowerDownCompletionCallbackRoutine
(
	_In_ PVOID pContext
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	CAdapterCommon *pAdapterCommon = NULL;

	DPF_ENTER("[CodecPowerDownCompletionCallbackRoutine]");

	if (NULL == pContext)
	{
		DPF(D_ERROR, "Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pAdapterCommon = (CAdapterCommon *)pContext;

	//KeSetEvent(&pAdapterCommon->m_CodecD3ReadyEvent, 0, FALSE);

Exit:
	DPF_ENTER("Exit with 0x%lx.", Status);
	return Status;
}
