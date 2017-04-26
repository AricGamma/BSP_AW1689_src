
#include "IRDecoder.h"



//PD6121G-F Decoder
VOID PD6121G_F_Decoder(_In_ ULONG* Data, _In_ UINT DataSize, _Out_ UINT8* AddressCode, _Out_ UINT8* DataCode)
{
	UINT32 tmp = 0;
	UINT8 data = 0;
	UINT8 dataNot = 0xFF;
	UINT8 addr = 0;
	UINT8 addrCompare = 0;

	if (DataSize < 64 + 3)
	{
		return;
	}

	for (UINT i = 0; i < 64; i+=2)
	{
		BOOL b = (Data[i + 4] / Data[i + 3]) >= 2 ? 1 : 0;
		tmp = tmp << 1;
		tmp |= b;
	}

	dataNot = tmp & 0xFF;
	tmp = tmp >> 8;
	data = tmp & 0xFF;
	tmp = tmp >> 8;
	addr = tmp & 0xFF;
	addrCompare = (UINT8)(tmp >> 8);

	if ((data & dataNot) == 0)
	{
		//reverse data bits
		{
			data = (data & 0x55) << 1 | (data & 0xAA) >> 1;
			data = (data & 0x33) << 2 | (data & 0xCC) >> 2;
			data = (data & 0x0F) << 4 | (data & 0xF0) >> 4;
		}
		*DataCode = data;
	}
	if (addr == addrCompare)
	{
		*AddressCode = addr;
	}

}