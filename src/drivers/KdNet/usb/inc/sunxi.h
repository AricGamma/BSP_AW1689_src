#ifndef  __SUNXI_H__
#define  __SUNXI_H__

#include <pshpack1.h>

#define DMA_BUFFER_SIZE         0x4000
#define DMA_BUFFERS             28
#define DMA_BUFFER_OFFSET       0

#define ENDPOINTS               8

#define OEM_BUFFER_SIZE         0x0100

#define HWEPTS                  32

#include  "../src/sunxi_usb/include/sunxi_usb_config.h"

typedef struct _DMA {

    //
    // Given the first endpoint head the controller expects to
    // find an array of endpoint heads.
    // The expected order is the receive endpoint head, followed
    // by the transmit endpoint head, followed by the same pair
    // for the next endpoint: EP0 receive, EP0 transmit, EP1 receive, etc.
    //
    // N.b. This array is expected to begin on a 4K boundary.
    //

    DECLSPEC_ALIGN(4096)
    UCHAR Buffers[DMA_BUFFERS][DMA_BUFFER_SIZE];

} DMA_MEMORY, *PDMA_MEMORY;

typedef struct _USBFNMP_CONTEXT {
    PVOID UsbCtrl;
    PDMA_MEMORY Dma;
    PVOID Gpio;

    PHYSICAL_ADDRESS DmaPa;

    BOOLEAN Attached;
    BOOLEAN Configured;
    BOOLEAN PendingDetach;
    BOOLEAN PendingReset;

    USBFNMP_BUS_SPEED Speed;

    ULONG FirstBuffer[ENDPOINTS];
    ULONG LastBuffer[ENDPOINTS];
    ULONG NextBuffer[DMA_BUFFERS];
    ULONG FreeBuffer;

//Controller
    USHORT VendorID;
    USHORT DeviceID;
    KD_NAMESPACE_ENUM NameSpace;
} USBFNMP_CONTEXT, *PUSBFNMP_CONTEXT;

#include <poppack.h>

#endif   //__SUNXI_H__

