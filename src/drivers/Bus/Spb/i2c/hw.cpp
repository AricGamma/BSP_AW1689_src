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

#include "internal.h"
#include "hw.tmh"

ULONG
HWREG<ULONG>::Read(
    VOID
    )
{
    volatile ULONG *addr = &m_Value;
    ULONG v = READ_REGISTER_ULONG((PULONG)addr);
    return v;
}

ULONG
HWREG<ULONG>::Write(
    _In_ ULONG Value
    )
{
    volatile ULONG *addr = &m_Value;
    WRITE_REGISTER_ULONG((PULONG)addr, Value);
    return Value;
}

USHORT
HWREG<USHORT>::Read(
    VOID
    )
{
    volatile USHORT *addr = &m_Value;
    USHORT v = READ_REGISTER_USHORT((PUSHORT)addr);
    return v;
}

USHORT
HWREG<USHORT>::Write(
    _In_ USHORT Value
    )
{
    volatile USHORT *addr = &m_Value;
    WRITE_REGISTER_USHORT((PUSHORT)addr, Value);
    return Value;
}

UCHAR
HWREG<UCHAR>::Read(
    VOID
    )
{
    volatile UCHAR *addr = &m_Value;
    UCHAR v = READ_REGISTER_UCHAR((PUCHAR)addr);
    return v;
}

UCHAR
HWREG<UCHAR>::Write(
    _In_ UCHAR Value
    )
{
    volatile UCHAR *addr = &m_Value;
    WRITE_REGISTER_UCHAR((PUCHAR)addr, Value);
    return Value;
}
