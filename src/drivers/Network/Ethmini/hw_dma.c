#include "Ethmini.h"
#include "hw_dma.tmh"

NDIS_STATUS HW_DMA_Init(PMAC Mac)
{	
	NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(Mac);

	return Status;
}

VOID HW_DMA_Init_Desc_Chain(PDMA_DESC desc, ULONG addr, ULONG size)
{
	/*
	 * In chained mode the desc3 points to the next element in the ring.
	 * The latest element has to point to the head.
	 */
	ULONG  i;
	PDMA_DESC p = desc;
	ULONG dma_phy = addr;

	for (i = 0; i < (size - 1); i++) {
		dma_phy += sizeof(DMA_DESC);
		p->desc3 = (unsigned int)dma_phy;
		/* Chain mode */
		p->desc1.all |= (1 << 24);
		p++;
	}
	p->desc1.all |= (1 << 24);
	p->desc3 = (unsigned int)addr;
}

