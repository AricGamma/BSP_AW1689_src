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

#ifndef _HW_H_
#define _HW_H_

template<typename T> struct HWREG
{
private: 

    //
    // Only one data member - this has to fit in the same space as the underlying type.
    //

    T m_Value;

public:

    T Read(void);
    T Write(_In_ T value);

    VOID 
    ReadBuffer(
        _In_                   ULONG BufferCe,
        _Out_writes_(BufferCe) T     Buffer[]
        )
    {
        for(ULONG i = 0; i < BufferCe; i++)
        {
            Buffer[i] = Read();
        }
    }

    VOID 
    WriteBuffer(
        _In_                   ULONG BufferCe,
        _Out_writes_(BufferCe) T     Buffer[]
        )
    {
        for(ULONG i = 0; i < BufferCe; i++)
        {
            Write(Buffer[i]);
        }
    }

    //
    // Operators with standard meanings.
    //

    T operator= (_In_ T value) {return Write(value);}
    T operator|=(_In_ T value) {return Write(Read() | value);}
    T operator&=(_In_ T value) {return Write(value | Read());}
      operator T()             {return Read();}

    //
    // Override the meaning of exclusive OR to mean clear
    //
    // Added this because x &= ~foo requires a cast of ~foo from signed int
    // back to the underlying (typically unsigned) type.  I would prefer x ~= foo 
    // but that's not a real C++ operator.
    //

    T operator^=(_In_ T value) {return Write(((T) ~value) & Read());}

    T SetBits  (_In_ T Flags) {return (*this |= Flags);}
    T ClearBits(_In_ T Flags) {return (*this &= ~Flags);}
    bool TestBits (_In_ T Flags) {return ((Read() & Flags) != 0)};
};

#endif
