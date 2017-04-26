/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_SileadTouch,
    0xabd3f5ae,0x345c,0x41d8,0xa2,0x71,0x65,0x76,0x84,0x42,0xea,0x9b);
// {abd3f5ae-345c-41d8-a271-65768442ea9b}
