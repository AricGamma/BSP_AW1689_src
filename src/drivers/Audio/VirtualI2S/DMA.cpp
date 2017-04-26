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
#include "DMA.h"
#include "Trace.h"
#include "DMA.tmh"
#include <math.h>

//#define DMA_LOOP_TRANSFER

EVT_WDF_PROGRAM_DMA I2SDmaTransmitProgramDma;
EVT_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE I2SDmaTransmitDmaTransferComplete;
EVT_WDF_PROGRAM_DMA I2SDmaReceiveProgramDma;
EVT_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE I2SDmaReceiveDmaTransferComplete;

NTSTATUS I2SDmaInitialize(
	_In_ PDEVICE_CONTEXT pDevExt)
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_DMA_ENABLER_CONFIG DmaEnablerConfig = { 0 };
	WDF_DMA_SYSTEM_PROFILE_CONFIG DmaSystemProfileConfig = { 0 };
	PHYSICAL_ADDRESS Address = { 0 };

	FunctionEnter();

	if (NULL == pDevExt) 
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	//
	// Initialize TX DMA
	//

	// Create DMA Enabler
	WDF_DMA_ENABLER_CONFIG_INIT(&DmaEnablerConfig,
		WdfDmaProfileSystem,
		DMA_MAX_TRANSFER_LENGTH_BYTES);

	Status = WdfDmaEnablerCreate(pDevExt->pDevice,
		&DmaEnablerConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&pDevExt->pTxDmaEnabler);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaEnablerCreate for TX failed with 0x%lx.", Status);
		goto Exit;
	}

	// Initialize System DMA Configuration
	Address.HighPart = 0;
	Address.LowPart = DMA_CODEC_FIFO_TX__ADDRESS;

	WdfDmaEnablerSetMaximumScatterGatherElements(pDevExt->pTxDmaEnabler, DMA_MAX_FRAGMENTS);

	WDF_DMA_SYSTEM_PROFILE_CONFIG_INIT(&DmaSystemProfileConfig,
		Address,
		Width16Bits,
		pDevExt->pDmaResourceDescriptor[0]);
#ifdef DMA_LOOP_TRANSFER
	DmaSystemProfileConfig.LoopedTransfer = TRUE;
#else
    DmaSystemProfileConfig.LoopedTransfer = FALSE;
#endif
	Status = WdfDmaEnablerConfigureSystemProfile(pDevExt->pTxDmaEnabler,
		&DmaSystemProfileConfig,
		WdfDmaDirectionWriteToDevice);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaEnablerConfigureSystemProfile for TX failed with 0x%lx.", Status);
		goto Exit;
	}

	// Create DMA Transaction
	Status = WdfDmaTransactionCreate(pDevExt->pTxDmaEnabler,
		WDF_NO_OBJECT_ATTRIBUTES,
		&pDevExt->pDmaTransmitTransaction);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaTransactionCreate for TX failed with 0x%lx.", Status);
		goto Exit;
	}

	// Save DMA Adapter information
	pDevExt->pWriteAdapter = WdfDmaEnablerWdmGetDmaAdapter(pDevExt->pTxDmaEnabler,
		WdfDmaDirectionWriteToDevice);
	if (NULL == pDevExt->pWriteAdapter)
	{
		DbgPrint_E("WdfDmaEnablerWdmGetDmaAdapter for TX failed.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	Status = pDevExt->pWriteAdapter->DmaOperations->GetDmaAdapterInfo(pDevExt->pWriteAdapter, &pDevExt->DmaWriteInfo);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("GetDmaAdapterInfo for TX failed with 0x%lx.", Status);
		goto Exit;
	}

	StateAdd(pDevExt->DmaState, DMA_STATE_TRANSMIT_CONFIGED);

	//
	// Initialize RX DMA
	//

	// Create DMA Enabler
	Status = WdfDmaEnablerCreate(pDevExt->pDevice,
		&DmaEnablerConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&(pDevExt->pRxDmaEnabler));
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaEnablerCreate for RX failed with 0x%lx.", Status);
		goto Exit;
	}

	// Initialize System DMA Configuration
	WdfDmaEnablerSetMaximumScatterGatherElements(pDevExt->pRxDmaEnabler, DMA_MAX_FRAGMENTS);

	Address.LowPart = DMA_CODEC_FIFO_RX__ADDRESS;
	
	WDF_DMA_SYSTEM_PROFILE_CONFIG_INIT(&DmaSystemProfileConfig,
		Address,
		Width16Bits,
		pDevExt->pDmaResourceDescriptor[1]);

	//DmaSystemProfileConfig.LoopedTransfer = TRUE;

	Status = WdfDmaEnablerConfigureSystemProfile(pDevExt->pRxDmaEnabler,
		&DmaSystemProfileConfig,
		WdfDmaDirectionReadFromDevice);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaEnablerConfigureSystemProfile for RX failed with 0x%lx.", Status);
		goto Exit;
	}

	// Create DMA Transaction
	Status = WdfDmaTransactionCreate(pDevExt->pRxDmaEnabler,
		WDF_NO_OBJECT_ATTRIBUTES,
		&pDevExt->pDmaReceiveTransaction);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaTransactionCreate for RX failed with 0x%lx.", Status);
		goto Exit;
	}

	// Save DMA Adapter information
	pDevExt->pReadAdapter = WdfDmaEnablerWdmGetDmaAdapter(pDevExt->pRxDmaEnabler,
		WdfDmaDirectionReadFromDevice);
	if (NULL == pDevExt->pReadAdapter)
	{
		DbgPrint_E("WdfDmaEnablerWdmGetDmaAdapter for RX failed.");
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	Status = pDevExt->pReadAdapter->DmaOperations->GetDmaAdapterInfo(pDevExt->pReadAdapter, &pDevExt->DmaReadInfo);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("GetDmaAdapterInfo for RX failed with 0x%lx.", Status);
		goto Exit;
	}

	StateAdd(pDevExt->DmaState, DMA_STATE_RECEIVE_CONFIGED);

Exit:
	FunctionExit(Status);
	return Status;
}

NTSTATUS I2SDmaTransmit(
	_In_ PDEVICE_CONTEXT pDevExt,
	_In_ PMDL pTransferMdl,
	_In_ ULONG TransferSize,
	_In_ ULONG NotificationsPerBuffer)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrint_V("Entered. NotificationsPerBuffer:%u , TransferSize:%u", NotificationsPerBuffer, TransferSize);
	pDevExt->TransferCounter=0;
	if ((NULL == pDevExt)
		|| (NULL == pTransferMdl))
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (0 == TransferSize)
	{
		DbgPrint_E("0 byte to be transfered.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (0 == NotificationsPerBuffer)
	{
		DbgPrint_E("0 notification count per buffer.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

	if (!StateTest(pDevExt->DmaState, DMA_STATE_TRANSMIT_CONFIGED)) 
	{
		DbgPrint_E("DMA transmit not configured.");
		Status = STATUS_INVALID_DEVICE_STATE;
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	if (StateTest(pDevExt->DmaState, DMA_STATE_TRANSMIT_STARTED))
	{
		DbgPrint_W("DMA transmit already enabled.");
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	NT_ASSERT(0 != pDevExt->DmaWriteInfo.V1.MinimumTransferUnit);

	pDevExt->pTransmitMdl = pTransferMdl;
	pDevExt->DmaTransmittedLength = 0;
	pDevExt->DmaTransmitTotalLength = TransferSize;
	pDevExt->DmaTransmitLengthPerOperation = pDevExt->DmaTransmitTotalLength / NotificationsPerBuffer;
	pDevExt->DmaInterruptCounter = 0;

	NT_ASSERT(0 == (pDevExt->DmaTransmitLengthPerOperation % pDevExt->DmaWriteInfo.V1.MinimumTransferUnit));
#ifdef DMA_LOOP_TRANSFER
	Status = WdfDmaTransactionInitialize(pDevExt->pDmaTransmitTransaction,
		I2SDmaTransmitProgramDma,
		WdfDmaDirectionWriteToDevice,
		pTransferMdl,
		MmGetMdlVirtualAddress(pTransferMdl),
		pDevExt->DmaTransmitTotalLength);
#else
	  Status = WdfDmaTransactionInitializeUsingOffset(pDevExt->pDmaTransmitTransaction,
		I2SDmaTransmitProgramDma,
		WdfDmaDirectionWriteToDevice,
		pTransferMdl,
		pDevExt->DmaTransmittedLength,
		pDevExt->DmaTransmitLengthPerOperation);
#endif
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaTransactionInitialize failed with 0x%lx.", Status);
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	WdfDmaTransactionSetDeviceAddressOffset(pDevExt->pDmaTransmitTransaction, (ULONG)DMA_TX_OFFSET);

	WdfDmaTransactionSetTransferCompleteCallback(pDevExt->pDmaTransmitTransaction,
		I2SDmaTransmitDmaTransferComplete,
		pDevExt);
	Status = WdfDmaTransactionExecute(pDevExt->pDmaTransmitTransaction, WDF_NO_CONTEXT);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaTransactionExecute failed with 0x%lx.", Status);
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	StateAdd(pDevExt->DmaState, DMA_STATE_TRANSMIT_STARTED);
	StateRemove(pDevExt->DmaState, DMA_STATE_TRANSMIT_CANCELLED);

	WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);

Exit:
	FunctionExit(Status);
	return Status;
}

BOOLEAN I2SDmaTransmitProgramDma(
	_In_  WDFDMATRANSACTION pTransaction,
	_In_  WDFDEVICE pDevice,
	_In_  WDFCONTEXT pContext,
	_In_  WDF_DMA_DIRECTION pDirection,
	_In_  PSCATTER_GATHER_LIST pSgList)
{
	UNREFERENCED_PARAMETER(pTransaction);
	UNREFERENCED_PARAMETER(pDevice);
	UNREFERENCED_PARAMETER(pContext);
	UNREFERENCED_PARAMETER(pDirection);
	UNREFERENCED_PARAMETER(pSgList);

	FunctionEnter();

	FunctionExit(STATUS_SUCCESS);
	return TRUE;
}

void I2SDmaTransmitDmaTransferComplete(
	_In_ WDFDMATRANSACTION     pDmaTransaction,
	_In_ WDFDEVICE             pWdfDevice,
	_In_ PVOID                 pContext,
	_In_ WDF_DMA_DIRECTION     Direction,
	_In_ DMA_COMPLETION_STATUS DmaStatus)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;
	ULONG BytesTransferred = 0;
	ULONG BytesNotTransferred = 0;
	BOOLEAN Result = FALSE;
	static ULONG PreBytesNotTransferred=0XFFFFFFFF;
	UNREFERENCED_PARAMETER(pWdfDevice);
	UNREFERENCED_PARAMETER(Direction);

	//FunctionEnter();
	// For the interrupt timestamp test
	//LARGE_INTEGER Time = { 0 };
	//KeQuerySystemTime(&Time);
	//DbgPrint_E("Current time 0x%lx - %lx", Time.HighPart, Time.LowPart);

	if ((NULL == pDmaTransaction)
		|| (NULL == pContext))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = (PDEVICE_CONTEXT)pContext;
	pDevExt->TransferCounter ++;
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Adpc=%d\n", pDevExt->TransferCounter); 
	BytesNotTransferred = pDevExt->pWriteAdapter->DmaOperations->ReadDmaCounter(pDevExt->pWriteAdapter);
#ifdef DMA_LOOP_TRANSFER
	BytesTransferred = pDevExt->DmaTransmitTotalLength - BytesNotTransferred;
#else
    BytesTransferred = pDevExt->DmaTransmitLengthPerOperation - BytesNotTransferred;
#endif
	DbgPrint_I("Bytes transferred %lu, remained %lu, Status %lu", BytesTransferred, BytesNotTransferred, DmaStatus);

	if (DmaComplete == DmaStatus)
	{
		WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

		if (!StateTest(pDevExt->DmaState, DMA_STATE_TRANSMIT_STARTED))
		{
			DbgPrint_I("Transmit DMA already stopped.");
			WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
			goto Exit;
		}
		
#ifndef DMA_LOOP_TRANSFER
		//
		// Simply assert the transferred bytes judgement now. Maybe some error process need to be added later.
		//
		NT_ASSERT(BytesTransferred == pDevExt->DmaTransmitLengthPerOperation);

		//
		// 1. Finally complete the transaction.
		// 2. Notify the audio driver that data is transferred.
		// 3. Start the transaction again. In current design, we will keep the DMA transaction on-going until the DMA is stopped.
		//
		Result = WdfDmaTransactionDmaCompleted(pDevExt->pDmaTransmitTransaction, &Status);
		NT_ASSERT(TRUE == Result);

		WdfDmaTransactionRelease(pDevExt->pDmaTransmitTransaction);
		pDevExt->pWriteAdapter->DmaOperations->FreeAdapterChannel(pDevExt->pWriteAdapter);
		
#else
		// DMA will notify the completion every interruption, so the data transferred is always a packet.
		BytesTransferred = pDevExt->DmaTransmitLengthPerOperation;
#endif		
		if ((NULL != pDevExt->DmaTransmitCallback.pCallbackRoutine)
			&& (NULL != pDevExt->DmaTransmitCallback.pCallbackContext))
		{
			pDevExt->DmaTransmitCallback.pCallbackRoutine(
				pDevExt->DmaTransmitCallback.pCallbackContext,
				BytesTransferred,
				STATUS_SUCCESS);
		}

#ifndef DMA_LOOP_TRANSFER
		pDevExt->DmaTransmittedLength = (pDevExt->DmaTransmittedLength + pDevExt->DmaTransmitLengthPerOperation) % pDevExt->DmaTransmitTotalLength;

		Status = WdfDmaTransactionInitializeUsingOffset(pDevExt->pDmaTransmitTransaction,
			I2SDmaTransmitProgramDma,
			WdfDmaDirectionWriteToDevice,
			pDevExt->pTransmitMdl,
			pDevExt->DmaTransmittedLength,
			pDevExt->DmaTransmitLengthPerOperation);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("WdfDmaTransactionInitialize failed with 0x%lx.", Status);
			WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
			goto Exit;
		}

		WdfDmaTransactionSetDeviceAddressOffset(pDevExt->pDmaTransmitTransaction, (ULONG)DMA_TX_OFFSET);

		WdfDmaTransactionSetTransferCompleteCallback(pDevExt->pDmaTransmitTransaction,
			I2SDmaTransmitDmaTransferComplete,
			pDevExt);

		Status = WdfDmaTransactionExecute(pDevExt->pDmaTransmitTransaction, WDF_NO_CONTEXT);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("WdfDmaTransactionExecute failed with 0x%lx.", Status);
		}
		
#endif
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
	}
	else if (DmaCancelled == DmaStatus)
	{
		WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

		if (StateTest(pDevExt->DmaState, DMA_STATE_TRANSMIT_CANCELLED))
		{
			DbgPrint_W("Transmit DMA already cancelled.");
			WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
			goto Exit;
		}

		Result = WdfDmaTransactionDmaCompletedFinal(pDevExt->pDmaTransmitTransaction, BytesTransferred, &Status);

		NT_ASSERT(TRUE == Result);

		WdfDmaTransactionRelease(pDevExt->pDmaTransmitTransaction);
		pDevExt->pWriteAdapter->DmaOperations->FreeAdapterChannel(pDevExt->pWriteAdapter);

		pDevExt->pTransmitMdl = NULL;
		pDevExt->DmaTransmitCallback.pCallbackContext = NULL;
		pDevExt->DmaTransmitCallback.pCallbackRoutine = NULL;

		StateAdd(pDevExt->DmaState, DMA_STATE_TRANSMIT_CANCELLED);
		StateRemove(pDevExt->DmaState, DMA_STATE_TRANSMIT_STARTED);

		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
	}
	else
	{
		// TODO: release transaction, any other action necessary to flush and free DMA line
		NT_ASSERTMSG("DMA transfer failure", FALSE);
	}

Exit:
	//FunctionExit(Status);
	return;
}

NTSTATUS I2SDmaReceive(
	_In_ PDEVICE_CONTEXT pDevExt,
	_In_ PMDL pTransferMdl,
	_In_ ULONG TransferSize,
	_In_ ULONG NotificationsPerBuffer)
{
	NTSTATUS Status = STATUS_SUCCESS;

	FunctionEnter();

	if ((NULL == pDevExt)
		|| (NULL == pTransferMdl))
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (0 == TransferSize)
	{
		DbgPrint_E("0 byte to be transfered.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (0 == NotificationsPerBuffer)
	{
		DbgPrint_E("0 notification per buffer.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

	if (!StateTest(pDevExt->DmaState, DMA_STATE_RECEIVE_CONFIGED))
	{
		DbgPrint_E("Receive DMA not configured.");
		Status = STATUS_INVALID_DEVICE_STATE;
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	if (StateTest(pDevExt->DmaState, DMA_STATE_RECEIVE_STARTED))
	{
		DbgPrint_W("Receive DMA already enabled.");
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	NT_ASSERT(0 != pDevExt->DmaReadInfo.V1.MinimumTransferUnit);

	pDevExt->pReceiveMdl = pTransferMdl;
	pDevExt->DmaReceivedLength = 0;
	pDevExt->DmaReceiveTotalLength = TransferSize;
	pDevExt->DmaReceiveLengthPerOperation = pDevExt->DmaReceiveTotalLength / NotificationsPerBuffer;

	NT_ASSERT(0 == (pDevExt->DmaReceiveLengthPerOperation % pDevExt->DmaReadInfo.V1.MinimumTransferUnit));

	Status = WdfDmaTransactionInitializeUsingOffset(pDevExt->pDmaReceiveTransaction,
		I2SDmaReceiveProgramDma,
		WdfDmaDirectionReadFromDevice,
		pTransferMdl,
		pDevExt->DmaReceivedLength,
		pDevExt->DmaReceiveLengthPerOperation);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaTransactionInitialize failed with 0x%lx.", Status);
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	WdfDmaTransactionSetDeviceAddressOffset(pDevExt->pDmaReceiveTransaction, (ULONG)DMA_RX_OFFSET);

	WdfDmaTransactionSetTransferCompleteCallback(pDevExt->pDmaReceiveTransaction,
		I2SDmaReceiveDmaTransferComplete,
		pDevExt);

	Status = WdfDmaTransactionExecute(pDevExt->pDmaReceiveTransaction, WDF_NO_CONTEXT);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint_E("WdfDmaTransactionExecute failed with 0x%lx.", Status);
		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
		goto Exit;
	}

	StateAdd(pDevExt->DmaState, DMA_STATE_RECEIVE_STARTED);
	StateRemove(pDevExt->DmaState, DMA_STATE_RECEIVE_CANCELLED);

	WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);

Exit:
	FunctionExit(Status);
	return Status;
}

BOOLEAN I2SDmaReceiveProgramDma(
	_In_  WDFDMATRANSACTION pTransaction,
	_In_  WDFDEVICE pDevice,
	_In_  WDFCONTEXT pContext,
	_In_  WDF_DMA_DIRECTION pDirection,
	_In_  PSCATTER_GATHER_LIST pSgList)
{
	UNREFERENCED_PARAMETER(pTransaction);
	UNREFERENCED_PARAMETER(pDevice);
	UNREFERENCED_PARAMETER(pContext);
	UNREFERENCED_PARAMETER(pDirection);
	UNREFERENCED_PARAMETER(pSgList);

	FunctionEnter();

	FunctionExit(STATUS_SUCCESS);
	return TRUE;
}

void I2SDmaReceiveDmaTransferComplete(
	_In_ WDFDMATRANSACTION     pDmaTransaction,
	_In_ WDFDEVICE             pWdfDevice,
	_In_ PVOID                 pContext,
	_In_ WDF_DMA_DIRECTION     Direction,
	_In_ DMA_COMPLETION_STATUS DmaStatus)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PDEVICE_CONTEXT pDevExt = NULL;
	ULONG BytesTransferred = 0;
	ULONG BytesNotTransferred = 0;
	BOOLEAN Result = FALSE;

	UNREFERENCED_PARAMETER(pWdfDevice);
	UNREFERENCED_PARAMETER(Direction);

	//FunctionEnter();

	if ((NULL == pDmaTransaction)
		|| (NULL == pContext))
	{
		DbgPrint_E("Invalid parameters.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	pDevExt = (PDEVICE_CONTEXT)pContext;

	BytesNotTransferred = pDevExt->pReadAdapter->DmaOperations->ReadDmaCounter(pDevExt->pReadAdapter);
	BytesTransferred = pDevExt->DmaReceiveLengthPerOperation - BytesNotTransferred;
	DbgPrint_V("Bytes transferred %ld, remained %ld, Status %ld", BytesTransferred, BytesNotTransferred, DmaStatus);

	if (DmaComplete == DmaStatus)
	{
		WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

		if (!StateTest(pDevExt->DmaState, DMA_STATE_RECEIVE_STARTED))
		{
			DbgPrint_W("Receive DMA already stopped.");
			WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
			goto Exit;
		}

		//
		// Simply assert the transferred bytes judgement now. Maybe some error process need to be added later.
		//
		NT_ASSERT(BytesTransferred == pDevExt->DmaReceiveLengthPerOperation);

		//
		// 1. Finally complete the transaction.
		// 2. Notify the audio driver that data is transferred.
		// 3. Start the transaction again. In current design, we will keep the DMA transaction on-going until the DMA is stopped.
		//
		Result = WdfDmaTransactionDmaCompleted(pDevExt->pDmaReceiveTransaction, &Status);

		NT_ASSERT(TRUE == Result);

		WdfDmaTransactionRelease(pDevExt->pDmaReceiveTransaction);
		pDevExt->pReadAdapter->DmaOperations->FreeAdapterChannel(pDevExt->pReadAdapter);

		if ((NULL != pDevExt->DmaReceiveCallback.pCallbackRoutine)
			&& (NULL != pDevExt->DmaReceiveCallback.pCallbackContext))
		{
			pDevExt->DmaReceiveCallback.pCallbackRoutine(pDevExt->DmaReceiveCallback.pCallbackContext,
				BytesTransferred,
				STATUS_SUCCESS);
		}

		pDevExt->DmaReceivedLength = (pDevExt->DmaReceivedLength + pDevExt->DmaReceiveLengthPerOperation) % pDevExt->DmaReceiveTotalLength;

		Status = WdfDmaTransactionInitializeUsingOffset(pDevExt->pDmaReceiveTransaction,
			I2SDmaReceiveProgramDma,
			WdfDmaDirectionReadFromDevice,
			pDevExt->pReceiveMdl,
			pDevExt->DmaReceivedLength,
			pDevExt->DmaReceiveLengthPerOperation);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("WdfDmaTransactionInitialize failed with 0x%lx.", Status);
			WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
			goto Exit;
		}

		WdfDmaTransactionSetDeviceAddressOffset(pDevExt->pDmaReceiveTransaction, (ULONG)DMA_RX_OFFSET);

		WdfDmaTransactionSetTransferCompleteCallback(pDevExt->pDmaReceiveTransaction,
			I2SDmaReceiveDmaTransferComplete,
			pDevExt);
		Status = WdfDmaTransactionExecute(pDevExt->pDmaReceiveTransaction, WDF_NO_CONTEXT);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint_E("WdfDmaTransactionExecute failed with 0x%lx.", Status);
		}

		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
	}
	else if (DmaCancelled == DmaStatus)
	{
		WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

		if (StateTest(pDevExt->DmaState, DMA_STATE_RECEIVE_CANCELLED)) 
		{
			DbgPrint_W("Receive DMA already cancelled.");
			WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
			goto Exit;
		}

		Result = WdfDmaTransactionDmaCompletedFinal(pDevExt->pDmaReceiveTransaction, BytesTransferred, &Status);

		NT_ASSERT(TRUE == Result);

		WdfDmaTransactionRelease(pDevExt->pDmaReceiveTransaction);
		pDevExt->pReadAdapter->DmaOperations->FreeAdapterChannel(pDevExt->pReadAdapter);

		pDevExt->pReceiveMdl = NULL;
		pDevExt->DmaReceiveCallback.pCallbackContext = NULL;
		pDevExt->DmaReceiveCallback.pCallbackRoutine = NULL;

		StateAdd(pDevExt->DmaState, DMA_STATE_RECEIVE_CANCELLED);
		StateRemove(pDevExt->DmaState, DMA_STATE_RECEIVE_STARTED);

		WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);
	}
	else
	{
		// TODO: release transaction, any other action necessary to flush and free DMA line
		NT_ASSERTMSG("DMA transfer failure", FALSE);
	}

Exit:

	//FunctionExit(Status);
	return;
}

NTSTATUS I2SDmaStop(
	_In_ PDEVICE_CONTEXT pDevExt,
	_In_ BOOL IsCapture)
{
	NTSTATUS Status = STATUS_SUCCESS;

	FunctionEnter();
	pDevExt->TransferCounter = 0;
	if (NULL == pDevExt) 
	{
		DbgPrint_E("Invalid parameter.");
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	WdfSpinLockAcquire(pDevExt->pDmaTransmissionLock);

	if (!IsCapture && StateTest(pDevExt->DmaState, DMA_STATE_TRANSMIT_STARTED))
	{
		StateRemove(pDevExt->DmaState, DMA_STATE_TRANSMIT_STARTED);
		WdfDmaTransactionStopSystemTransfer(pDevExt->pDmaTransmitTransaction);
	}
	else if (IsCapture && StateTest(pDevExt->DmaState, DMA_STATE_RECEIVE_STARTED))
	{
		StateRemove(pDevExt->DmaState, DMA_STATE_RECEIVE_STARTED);
		WdfDmaTransactionStopSystemTransfer(pDevExt->pDmaReceiveTransaction);
	}
	else
	{
		DbgPrint_V("Dma has been stoped. (IsCapture:%x)", IsCapture);
	}

	WdfSpinLockRelease(pDevExt->pDmaTransmissionLock);

	//
	// TODO: Stop DMA HW here
	//

Exit:
	FunctionExit(Status);
	return Status;
}