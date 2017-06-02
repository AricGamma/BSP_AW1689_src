/** @file
*
*  Copyright (c) 2007-2014, Allwinner Technology Co., Ltd. All rights reserved.
*  http://www.allwinnertech.com
*
*  Leeway Zhang <zhangliwei@allwinnertech.com>
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

Device(COM0)
{
    Name(_HID, "AWTH0009")                                
    Name(_UID, 0x0)     
    Method (_STA, 0, NotSerialized) 
    {
        Return (0x0F)
    }                  
    Name(_CRS, ResourceTemplate ()
    {
        MEMORY32FIXED(ReadWrite, 0x01c28000, 0x400, )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) {32}
        UARTSerialBus( 
            115200,                 // InitialBaudRate: in BPS 
            ,                       // BitsPerByte: default to 8 bits 
            ,                       // StopBits: Defaults to one bit 
            0x00,                   // LinesInUse: 8 1bit flags to 
                                    //   declare enabled control lines. 
                                    //   Raspberry Pi2 does not exposed 
                                    //   HW control signals > not supported. 
                                    //   Optional bits: 
                                    //   - Bit 7 (0x80) Request To Send (RTS) 
                                    //   - Bit 6 (0x40) Clear To Send (CTS) 
                                    //   - Bit 5 (0x20) Data Terminal Ready (DTR) 
                                    //   - Bit 4 (0x10) Data Set Ready (DSR) 
                                    //   - Bit 3 (0x08) Ring Indicator (RI) 
                                    //   - Bit 2 (0x04) Data Carrier Detect (DTD) 
                                    //   - Bit 1 (0x02) Reserved. Must be 0. 
                                    //   - Bit 0 (0x01) Reserved. Must be 0. 
            ,                       // IsBigEndian: 
                                    //   default to LittleEndian. 
            ,                       // Parity: Defaults to no parity 
            ,                       // FlowControl: Defaults to 
                                    //   no flow control. 
            64,                     // ReceiveBufferSize 
            64,                     // TransmitBufferSize 
            "\\_SB.COM0",           // ResourceSource: dummy device node, any device in ACPI table seems ok 
            ,                       // ResourceSourceIndex: assumed to be 0 
            ,                       // ResourceUsage: assumed to be 
                                    //   ResourceConsumer 
            COM0,                   // DescriptorName: creates name 
                                    //   for offset of resource descriptor 
        )                      // Vendor data 
    })
    Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
    {
        Return (Zero)
    }
}//End of Device 'COM0'

Device(COM1)
{
    Name(_HID, "AWTH0009")                                
    Name(_UID, 0x1)     
    Method (_STA, 0, NotSerialized) 
    {
        Return (0x0F)
    }                  
    Name(_CRS, ResourceTemplate ()
    {
        MEMORY32FIXED(ReadWrite, 0x01c28400, 0x400, )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) {33}
        UARTSerialBus( 
            115200,                 // InitialBaudRate: in BPS 
            ,                       // BitsPerByte: default to 8 bits 
            ,                       // StopBits: Defaults to one bit 
            0x00,                   // LinesInUse: 8 1bit flags to 
                                    //   declare enabled control lines. 
                                    //   Raspberry Pi2 does not exposed 
                                    //   HW control signals > not supported. 
                                    //   Optional bits: 
                                    //   - Bit 7 (0x80) Request To Send (RTS) 
                                    //   - Bit 6 (0x40) Clear To Send (CTS) 
                                    //   - Bit 5 (0x20) Data Terminal Ready (DTR) 
                                    //   - Bit 4 (0x10) Data Set Ready (DSR) 
                                    //   - Bit 3 (0x08) Ring Indicator (RI) 
                                    //   - Bit 2 (0x04) Data Carrier Detect (DTD) 
                                    //   - Bit 1 (0x02) Reserved. Must be 0. 
                                    //   - Bit 0 (0x01) Reserved. Must be 0. 
            ,                       // IsBigEndian: 
                                    //   default to LittleEndian. 
            ,                       // Parity: Defaults to no parity 
            ,                       // FlowControl: Defaults to 
                                    //   no flow control. 
            64,                     // ReceiveBufferSize 
            64,                     // TransmitBufferSize 
            "\\_SB.COM1",           // ResourceSource: dummy device node, any device in ACPI table seems ok 
            ,                       // ResourceSourceIndex: assumed to be 0 
            ,                       // ResourceUsage: assumed to be 
                                    //   ResourceConsumer 
            COM1,                   // DescriptorName: creates name 
                                    //   for offset of resource descriptor 
        )                      // Vendor data 
    })
    Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
    {
        Return (Zero)
    }
}//End of Device 'COM1'

Device(COM2)
{
    Name(_HID, "AWTH0009")                                
    Name(_UID, 0x2)     
    Method (_STA, 0, NotSerialized) 
    {
        Return (0x0F)
    }                  
    Name(_CRS, ResourceTemplate ()
    {
        MEMORY32FIXED(ReadWrite, 0x01c28800, 0x400, )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) {34}
        UARTSerialBus( 
            115200,                 // InitialBaudRate: in BPS 
            ,                       // BitsPerByte: default to 8 bits 
            ,                       // StopBits: Defaults to one bit 
            0x00,                   // LinesInUse: 8 1bit flags to 
                                    //   declare enabled control lines. 
                                    //   Raspberry Pi2 does not exposed 
                                    //   HW control signals > not supported. 
                                    //   Optional bits: 
                                    //   - Bit 7 (0x80) Request To Send (RTS) 
                                    //   - Bit 6 (0x40) Clear To Send (CTS) 
                                    //   - Bit 5 (0x20) Data Terminal Ready (DTR) 
                                    //   - Bit 4 (0x10) Data Set Ready (DSR) 
                                    //   - Bit 3 (0x08) Ring Indicator (RI) 
                                    //   - Bit 2 (0x04) Data Carrier Detect (DTD) 
                                    //   - Bit 1 (0x02) Reserved. Must be 0. 
                                    //   - Bit 0 (0x01) Reserved. Must be 0. 
            ,                       // IsBigEndian: 
                                    //   default to LittleEndian. 
            ,                       // Parity: Defaults to no parity 
            ,                       // FlowControl: Defaults to 
                                    //   no flow control. 
            64,                     // ReceiveBufferSize 
            64,                     // TransmitBufferSize 
            "\\_SB.COM2",           // ResourceSource: dummy device node, any device in ACPI table seems ok 
            ,                       // ResourceSourceIndex: assumed to be 0 
            ,                       // ResourceUsage: assumed to be 
                                    //   ResourceConsumer 
            COM2,                   // DescriptorName: creates name 
                                    //   for offset of resource descriptor 
        )                      // Vendor data 
    })
    Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
    {
        Return (Zero)
    }
}//End of Device 'COM2'

Device(COM3)
{
    Name(_HID, "AWTH0009")                                
    Name(_UID, 0x3)     
    Method (_STA, 0, NotSerialized) 
    {
        Return (0x0F)
    }                  
    Name(_CRS, ResourceTemplate ()
    {
        MEMORY32FIXED(ReadWrite, 0x01c28C00, 0x400, )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) {35}
        UARTSerialBus( 
            115200,                 // InitialBaudRate: in BPS 
            ,                       // BitsPerByte: default to 8 bits 
            ,                       // StopBits: Defaults to one bit 
            0x00,                   // LinesInUse: 8 1bit flags to 
                                    //   declare enabled control lines. 
                                    //   Raspberry Pi2 does not exposed 
                                    //   HW control signals > not supported. 
                                    //   Optional bits: 
                                    //   - Bit 7 (0x80) Request To Send (RTS) 
                                    //   - Bit 6 (0x40) Clear To Send (CTS) 
                                    //   - Bit 5 (0x20) Data Terminal Ready (DTR) 
                                    //   - Bit 4 (0x10) Data Set Ready (DSR) 
                                    //   - Bit 3 (0x08) Ring Indicator (RI) 
                                    //   - Bit 2 (0x04) Data Carrier Detect (DTD) 
                                    //   - Bit 1 (0x02) Reserved. Must be 0. 
                                    //   - Bit 0 (0x01) Reserved. Must be 0. 
            ,                       // IsBigEndian: 
                                    //   default to LittleEndian. 
            ,                       // Parity: Defaults to no parity 
            ,                       // FlowControl: Defaults to 
                                    //   no flow control. 
            64,                     // ReceiveBufferSize 
            64,                     // TransmitBufferSize 
            "\\_SB.COM3",           // ResourceSource: dummy device node, any device in ACPI table seems ok 
            ,                       // ResourceSourceIndex: assumed to be 0 
            ,                       // ResourceUsage: assumed to be 
                                    //   ResourceConsumer 
            COM3,                   // DescriptorName: creates name 
                                    //   for offset of resource descriptor 
        )                      // Vendor data 
    })
    Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
    {
        Return (Zero)
    }
}//End of Device 'COM3'

Device(COM4)
{
    Name(_HID, "AWTH0009")                                
    Name(_UID, 0x4)     
    Method (_STA, 0, NotSerialized) 
    {
        Return (0x0F)
    }                  
    Name(_CRS, ResourceTemplate ()
    {
        MEMORY32FIXED(ReadWrite, 0x01c29000, 0x400, )
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive, , , ) {36}
        UARTSerialBus( 
            115200,                 // InitialBaudRate: in BPS 
            ,                       // BitsPerByte: default to 8 bits 
            ,                       // StopBits: Defaults to one bit 
            0x00,                   // LinesInUse: 8 1bit flags to 
                                    //   declare enabled control lines. 
                                    //   Raspberry Pi2 does not exposed 
                                    //   HW control signals > not supported. 
                                    //   Optional bits: 
                                    //   - Bit 7 (0x80) Request To Send (RTS) 
                                    //   - Bit 6 (0x40) Clear To Send (CTS) 
                                    //   - Bit 5 (0x20) Data Terminal Ready (DTR) 
                                    //   - Bit 4 (0x10) Data Set Ready (DSR) 
                                    //   - Bit 3 (0x08) Ring Indicator (RI) 
                                    //   - Bit 2 (0x04) Data Carrier Detect (DTD) 
                                    //   - Bit 1 (0x02) Reserved. Must be 0. 
                                    //   - Bit 0 (0x01) Reserved. Must be 0. 
            ,                       // IsBigEndian: 
                                    //   default to LittleEndian. 
            ,                       // Parity: Defaults to no parity 
            ,                       // FlowControl: Defaults to 
                                    //   no flow control. 
            64,                     // ReceiveBufferSize 
            64,                     // TransmitBufferSize 
            "\\_SB.COM4",           // ResourceSource: dummy device node, any device in ACPI table seems ok 
            ,                       // ResourceSourceIndex: assumed to be 0 
            ,                       // ResourceUsage: assumed to be 
                                    //   ResourceConsumer 
            COM4,                   // DescriptorName: creates name 
                                    //   for offset of resource descriptor 
        )                      // Vendor data 
    })
    Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
    {
        Return (Zero)
    }
}//End of Device 'COM4'