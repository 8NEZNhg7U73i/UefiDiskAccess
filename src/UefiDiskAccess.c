/*
	UefiDiskAccess - Simple demo of Accessing Disk I/O

	Copyright 2021 Zero Tang. All rights reserved.

	This program is distributed in the hope that it will be useful, but
	without any warranty (no matter implied warranty or merchantability
	or fitness for a particular purpose, etc.).

	File Location: /src/efimain.c
*/

#include <Uefi.h>
#include <IndustryStandard/Mbr.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <intrin.h>
#include "UefiDiskAccess.h"
#include <Library/ShellLib.h>
#include <Guid/GlobalVariable.h>

CHAR16 BlockUntilKeyStroke(IN CHAR16 Unicode)
{
	EFI_INPUT_KEY InKey;
	do
	{
		UINTN fi = 0;
		gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &fi);
		gST->ConIn->ReadKeyStroke(gST->ConIn, &InKey);
	} while (InKey.UnicodeChar != Unicode && Unicode);
	return InKey.UnicodeChar;
}

CHAR16 BlockUntilAnyKeyStroke()
{
	EFI_INPUT_KEY InKey;
	UINTN fi = 0;
	gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &fi);
	gST->ConIn->ReadKeyStroke(gST->ConIn, &InKey);
	return InKey.UnicodeChar;
}

INTN EfiCompareGuid(EFI_GUID *Guid1, EFI_GUID *Guid2)
{
	if (Guid1->Data1 > Guid2->Data1)
		return 1;
	else if (Guid1->Data1 < Guid2->Data1)
		return -1;
	if (Guid1->Data2 > Guid2->Data2)
		return 1;
	else if (Guid1->Data2 < Guid2->Data2)
		return -1;
	if (Guid1->Data3 > Guid2->Data3)
		return 1;
	else if (Guid1->Data3 < Guid2->Data3)
		return -1;
	for (UINT8 i = 0; i < 8; i++)
	{
		if (Guid1->Data4[i] > Guid2->Data4[i])
			return 1;
		else if (Guid1->Data4[i] < Guid2->Data4[i])
			return -1;
	}
	return 0;
}

EFI_STATUS EnablePageBreak()
{
	EFI_SHELL_PROTOCOL *ShellProtocol;
	EFI_STATUS STATUS = gBS->LocateProtocol(
			&gEfiShellProtocolGuid,
			NULL,
			(VOID **)&ShellProtocol);
	if (STATUS == EFI_SUCCESS)
	{
		ShellProtocol->EnablePageBreak();
	}
	return EFI_SUCCESS;
}
/*
void SetGraphicsMode()
{
	UINT32 MaxHeight = 0;
	UINT32 MaxRow = 0;
	//UINTN OptIndex;
	EFI_STATUS STATUS;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphOut;
	EFI_HANDLE *GraphHandles;
	UINTN SizeOfInfo;
	UINTN GraphCount;
	UINTN CurrentGraphCount;
	UINT32 MaxMode;
	UINT32 CurrentMode;
	EFI_DEVICE_PATH *DevicePath;
	CHAR16 *StrPath;
	//UINTN i;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GraphInfo;
	//EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *GraphMode;
	STATUS = gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &GraphCount, &GraphHandles);
	Print(L"GraphCount:%d\n", GraphCount);
	for (CurrentGraphCount = 0; CurrentGraphCount < GraphCount; CurrentGraphCount++)
	{
		Print(L"CurrentGraphCount:%d\n", CurrentGraphCount);
		STATUS = gBS->HandleProtocol(GraphHandles[CurrentGraphCount], &gEfiGraphicsOutputProtocolGuid, &GraphOut);
		DevicePath = DevicePathFromHandle(GraphHandles[CurrentGraphCount]);
		StrPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
		Print(L"Graphics %d Device Path:%s\n", CurrentGraphCount, StrPath);
		MaxMode = GraphOut->Mode->MaxMode;
		CurrentMode = GraphOut->Mode->Mode;
		Print(L"MaxMode:%d, Mode:%d\n", MaxMode, CurrentMode);
		for (CurrentMode = 0; CurrentMode < MaxMode; CurrentMode++)
		{
			STATUS = GraphOut->QueryMode(GraphOut,(UINT32) CurrentMode, &SizeOfInfo, &GraphInfo);
			Print(L"Graphics Mode %d, PixelFormat %d:[%d,%d]\n", CurrentMode, GraphInfo->PixelFormat, GraphInfo->HorizontalResolution, GraphInfo->VerticalResolution);
			if (GraphInfo->HorizontalResolution > MaxHeight)
			{
				MaxHeight = GraphInfo->HorizontalResolution;
			}
			if (GraphInfo->VerticalResolution > MaxRow)
			{
				MaxRow = GraphInfo->VerticalResolution;
			}
		}
		Print(L"MaxHeight:%d, MaxRow:%d\n",MaxHeight, MaxRow);
	}
}
*/
#define SetRow 31
#define SetColumn 100

void SetConsoleMode()
{
	UINTN Row;
	UINTN Column;
	UINTN CurrentMode;
	EFI_STATUS STATUS;
	for (CurrentMode = 0; CurrentMode < gST->ConOut->Mode->MaxMode; CurrentMode++)
	{
		STATUS = gST->ConOut->QueryMode(gST->ConOut, CurrentMode, &Column, &Row);
		if (STATUS == EFI_SUCCESS)
		{
			Print(L"Column:%d, Row:%d\n", Column, Row);
			if (Row >= SetRow && Column >= SetColumn)
			{
				//gST->ConOut->SetMode(gST->ConOut, i);
				gST->ConOut->ClearScreen(gST->ConOut);
				gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
				//break;
			}
		}
	}
}

void SetConsoleModeToMaximumRows()
{
	UINTN MaxHgt = 0, OptIndex;
	for (UINTN i = 0; i < gST->ConOut->Mode->MaxMode; i++)
	{
		UINTN Col, Row;
		EFI_STATUS STATUS = gST->ConOut->QueryMode(gST->ConOut, i, &Col, &Row);
		if (STATUS == EFI_SUCCESS)
		{
			if (Row > MaxHgt)
			{
				OptIndex = i;
				MaxHgt = Row;
			}
		}
	}
	gST->ConOut->SetMode(gST->ConOut, OptIndex);
	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
}

void DisplaySize(IN UINT64 Size, OUT CHAR16 *Buffer, IN UINTN Limit)
{
	if (Size < LimitKiB)
		UnicodeSPrint(Buffer, Limit, L"%u Bytes", Size);
	else if (Size >= LimitKiB && Size < LimitMiB)
		UnicodeSPrint(Buffer, Limit, L"%u KiB", Size >> 10);
	else if (Size >= LimitMiB && Size < LimitGiB)
		UnicodeSPrint(Buffer, Limit, L"%u MiB", Size >> 20);
	else
		UnicodeSPrint(Buffer, Limit, L"%u GiB", Size >> 30);
}

EFI_STATUS FindMbrBlockDevice(IN MBR_PARTITION_RECORD *Mbr, OUT EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol)
{
	EFI_STATUS STATUS;
	UINTN DiskIndex;
	//EFI_LBA StartingLBA;
	//EFI_LBA	EndingLBA;
	for (DiskIndex = 0 ; DiskIndex < NumberOfDiskDevices; DiskIndex++)
	{
		STATUS = ComPareMem(&DiskDevices[DiskIndex]->PartInfo->Info->Mbr, Mbr, sizeof(MBR_PARTITION_RECORD));
		if (STATUS == EFI_SUCCESS)
		{
			BlockIoProtocol = DiskDevices[DiskIndex]->BlockIo;
			return EFI_SUCCESS;
		}
	}
}

EFI_STATUS FindGptBlockDevice(IN EFI_PARTITION_ENTRY *Gpt, OUT EFI_BLOCK_IO_PROTOCOL *BlockIoPrrotocol)
{
	EFI_STATUS STATUS;
	UINTN DiskIndex;
	//EFI_LBA StartingLBA;
	//EFI_LBA EndingLBA;
	for (DiskIndex = 0 ; DiskIndex < NumberOfDiskDevices; DiskIndex++)
	{
		STATUS = ComPareMem(&DiskDevices[DiskIndex]->PartInfo->Info->Gpt, Gpt, sizeof(MBR_PARTITION_RECORD));
		if (STATUS == EFI_SUCCESS)
		{
			BlockIoProtocol = DiskDevices[DiskIndex]->BlockIo;
			return EFI_SUCCESS;
		}
	}}

EFI_STATUS EnumMbrDisk(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol, OUT BOOLEAN IsGpt);
{
	EFI_STATUS STATUS;
	MASTER_BOOT_RECORD *MBRContent;
	MBR_PARTITION_RECORD *MbrPart;
	UINT32 StartingLBA, SizeInLBA, EndingLBA;
	CHAR16 ScaledStart[32], ScaledEnd[32], ScaledSize[32];
	EFI_BLOCO_IO_PROTOCOL *BlockIoProtocol;
	UINTN MbrPartIndex;

	IsGpt = FALSE;
	STATUS = BlockIoProtocol->ReadBlocks(BlockIoProtocol, BlockIoProtocol->Media->MediaId, 0, BlockIoProtocol->Media->BlockSize, MBRContent);
	if (STATUS == EFI_SUCCESS)
	{
		if (MBRContent->Signature != MBR_SIGNATURE)
		{
			Print(L"Invalid MBR Signature! MBR might be corrupted!\r\n");
			return EFI_DEVICE_ERROR;
		}
		for (MbrPartIndex = 0; MbrPartIndex < MAX_MBR_PARTITIONS; MbrPartIndex++)
		{
			MbrPart = &MBRContent->Partition[MbrPartIndex];
			if (MbrPart->OSIndicator)
			{
				UINT32 StartingLBA = *(UINT32 *)MbrPart->StartingLBA;
				UINT32 SizeInLBA = *(UINT32 *)MbrPart->SizeInLBA;
				UINT32 EndingLBA = (StartingLBA + SizeInLBA - 1);
				if (MbrPart->OSIndicator == PMBR_GPT_PARTITION || MbrPart->OSIndicator == EFI_PARTITION)
				{
					IsGpt = TRUE;
				}
				else
				{
					DisplaySize(__emulu(StartingLBA, BlockIoProtocol->Media->BlockSize), ScaledStart, sizeof(ScaledStart));
					DisplaySize(__emulu(EndingLBA, BlockIoProtocol->Media->BlockSize), ScaledEnd, sizeof(ScaledEnd));
					DisplaySize(__emulu(SizeInLBA, BlockIoProtocol->Media->BlockSize), ScaledSize, sizeof(ScaledSize));
					STATUS = FindMbrBlockDevice(MbrPart, BlockIoProtocol);
					if (STATUS == EFI_SUCCESS)
					{
						if (SizeInLBA == 0xFFFFFFFF)
						{
							Print(L"MBR Part %d, Block Device %d : StartLBA: %u EndLBA: %s OS Type: 0x%02X Size: Over 2TiB\n", MbrPartIndex, k, StartingLBA, EndingLBA, MbrPart->OSIndicator);
						}
						else
						{
							Print(L"MBR Part %d, Block Device %d : StartLBA: %u EndLBA: %u LBASize: %u OS Type: 0x%02X Size: %s\n", MbrPartIndex, k, StartingLBA, EndingLBA, SizeInLBA, ScaledSize, MbrPart->OSIndicator);
						}
					}
					if (SizeInLBA == 0xFFFFFFFF)
					{
						Print(L"MBR Part %d: OS Type: 0x%02X StartLBA: %u EndLBA: %s Size: Over 2TiB\n", MbrPartIndex, MbrPart->OSIndicator, StartingLBA, ScaledEnd);
					}
					else
					{
						Print(L"MBR Part %d: OS Type: 0x%02X StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n", MbrPartIndex, MbrPart->OSIndicator, StartingLBA, EndingLBA, SizeInLBA, ScaledSize);
					}
				}
			}
		}
	}
	return STATUS;
}

EFI_STATUS EnumGptDisk(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol)
{
	EFI_STATUS STATUS;
	EFI_PARTITION_TABLE_HEADER *GptHeader;
	EFI_PARTITION_ENTRY *PartitionEntry;
	UINT32 

	STATUS = BlockIoProtocol->ReadBlocks(BlockIoProtocol, BlockIoProtocol->Media->MediaId, StartLBA, BlockIoProtocol->Media->BlockSize, GptHeader);
	if (STATUS == EFI_SUCCESS)
	{
		if (GptHeader->Header.Signature != EFI_PTAB_HEADER_ID)
		{
			Print(L"Improper GPT Header Signature!");
			return EFI_DEVICE_ERROR;
		}
		else
		{
			Print(L"GPT Header Detected! First usable LBA: %u. Last usable LBA: %u.\n", GptHeader->FirstUsableLBA, GptHeader->LastUsableLBA);
			UINT32 PartitionEntrySize = GptHeader->SizeOfPartitionEntry * GptHeader->NumberOfPartitionEntries;
			VOID *PartitionEntries = AllocatePool(PartitionEntrySize);
			Print(L"Disk GUID: {%g} Max number of partitions: %u\n", &GptHeader->DiskGUID, GptHeader->NumberOfPartitionEntries);
			if (PartitionEntries)
			{
				STATUS = BlockIoProtocol->ReadBlocks(BlockIoProtocol, BlockIoProtocol->Media->MediaId, GptHeader->PartitionEntryLBA, PartitionEntrySize, PartitionEntries);
				if (STATUS == EFI_SUCCESS)
				{
					for (UINT32 j = 0; j < GptHeader->NumberOfPartitionEntries; j++)
					{
						PartitionEntry = (EFI_PARTITION_ENTRY *)((UINTN)PartitionEntries + j * GptHeader->SizeOfPartitionEntry);
						if (EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid))
						{
							DisplaySize(MultU64x32(PartitionEntry->StartingLBA, BlockIoProtocol->Media->BlockSize), ScaledStart, sizeof(ScaledStart));
							DisplaySize(MultU64x32(PartitionEntry->EndingLBA, BlockIoProtocol->Media->BlockSize), ScaledEnd, sizeof(ScaledEnd));
							DisplaySize(MultU64x32(PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, BlockIoProtocol->Media->BlockSize), ScaledSize, sizeof(ScaledSize));
							for (UINT32 k = 0; k < NumberOfDiskDevices; k++)
							{
								if (DiskDevices[k].DevicePath)
								{
									STATUS = FindGptSignature(DiskDevices[k].DevicePath, &PartitionEntry->UniquePartitionGUID);
									if (STATUS == EFI_SUCCESS)
									{
										Print(L"GPT Part %u, Block Device %u : StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n", j, k, PartitionEntry->StartingLBA, PartitionEntry->EndingLBA, PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, ScaledSize);
										break;
									}
									if (k == NumberOfDiskDevices - 1)
									{
										Print(L"GPT Part %u : StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n", j, PartitionEntry->StartingLBA, PartitionEntry->EndingLBA, PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, ScaledSize);
										break;
									}
								}
							}
							if (!EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeSystemPartGuid))
							{
								Print(L"Part Type : efi\n");
							}
							else if (!EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeMsReservedPartGuid))
							{
								Print(L"Part Type : msr\n");
							}
							else if (!EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeBasicDataPartGuid))
							{
								Print(L"Part Type : data\n");
							}
							else if (!EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeMsRecoveryPartGuid))
							{
								Print(L"Part Type : wre\n");
							}
							else
							{
								Print(L"Part Type GUID:    {%g}\n", &PartitionEntry->PartitionTypeGUID);
							}
							Print(L"Unique Part GUID:  {%g}\n", &PartitionEntry->UniquePartitionGUID);
						}
					}
				}
				FreePool(PartitionEntries);
			}
		}
	}
		else
		{
			Print(L"Failed to read GPT Header! STATUS=0x%r\n", STATUS);
		}
		FreePool(GptHeader);
	}



EFI_STATUS EnumDiskPartitions(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol)
{
	EFI_STATUS STATUS;
	BOOLEAN IsGpt;
	if (!BlockIoProtocol->Media->LogicalPartition)
	{
		STATUS = EnumMbrDisk(BlockIoProtocol, IsGpt);
		if (IsGpt)
		{
			STATUS = EnumGptDisk(BlockIoProtocol);
		}
	}
}

void EnumAllDiskPartitions()
{
	UINTN DiskDeviceIndex;
	for (UINTN DiskDeviceIndex = 0; DiskDeviceIndex < NumberOfDiskDevices; DiskDeviceIndex++)
	{
		// Skip absent media and partition media.
		if (DiskDevices[DiskDeviceIndex]->BlockIo->Media->MediaPresent && !DiskDevices[DiskDeviceIndex]->BlockIo->Media->LogicalPartition)
		{
			CHAR16 *DiskDevicePath = ConvertDevicePathToText(DiskDevices[DiskDeviceIndex]->DevicePath, FALSE, FALSE);
			Print(L"=============================================================================\r\n");
			Print(L"Part Info of Block Device %u Path: %s\n", DiskDeviceIndex, DiskDevicePath);
			FreePool(DiskDevicePath);
			Print(L"Disk Last LBA: %u.\n", DiskDevices[DiskDeviceIndex]->BlockIo->Media->LastBlock);
			EnumDiskPartitions(DiskDevices[DiskDeviceIndex]->BlockIo);
		}
	}
	Print(L"=============================================================================\r\n");
}

/*
EFI_STATUS FindGptSignature(CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath, EFI_GUID *GptSignature)
{
	CHAR16 *DevPath;
	CONST HARDDRIVE_DEVICE_PATH *DevicePathMask;
	if (!DevicePath || !GptSignature)
	{
		return EFI_INVALID_PARAMETER;
	}
	DevPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	//Print(L"DevPath: %s\n", DevPath);
	while (!IsDevicePathEnd(DevicePath))
	{
		DevicePathMask = (CONST HARDDRIVE_DEVICE_PATH *)DevicePath;
		DevicePath = NextDevicePathNode(DevicePath);
		//DevPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
		//Print(L"DevPath: %s\n", DevPath);
		if (DevicePathMask->Header.Type != MEDIA_DEVICE_PATH)
		{
			continue;
		}
		// Check if the device path describes a GPT partition or disk
		if (DevicePathMask->SignatureType != 2)
		{
			continue;
		}
		if (!CompareMem(GptSignature, DevicePathMask->Signature, sizeof(EFI_GUID)))
		{
			return EFI_SUCCESS;
		}
	}
	return EFI_NOT_FOUND;
}
*/

//EFI_STATUS DevicePathConvert(CONST IN EFI_DEVICE_PATH_PROTOCOL *DevicePath, IN EFI_BLOCK_IO_PROTOCOL *BlockIo, IN EFI_PARTITION_INFORMATION_PROTOCOL *PartitionInfo)
EFI_STATUS DevicePathConvert(IN DiskDevices *DiskDevices)
{
	CONST HARDDRIVE_DEVICE_PATH *DevicePathMask;
	EFI_LBA LastBlock;
	EFI_LBA StartingLBA;
	EFI_LBA EndingLBA;
	EFI_GUID PartitionTypeGUID;
	EFI_GUID UniquePartitionGUID;
	EFI_LBA PartitionStart;
	EFI_LBA PartitionSize;
	UINT32 PartitionNumber;
	UINT8 SignatureType;
	BOOLEAN IsDisk;
	EFI_DEVICE_PATH_PROTOCOL *DevicePath = DiskDevices->DevicePath;
	//EFI_BLOCK_IO_PROTOCOL *BlockIO = DiskDevices->BlockIo;
	EFI_PARTITION_INFORMATION_PROTOCOL *PartitionInfo = DiskDevices->PartInfo;

	if (!DevicePath)
	{
		return EFI_INVALID_PARAMETER;
	}
	//CHAR16 *DevPath;

	if (PartitionInfo && DevicePathMask->SignatureType)
	{
		IsDisk = TRUE;
		return EFI_SUCCESS;
	}
	else
	{
		IsDisk = FALSE;
	}
	
	//DevPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	//Print(L"DevPath: %s\n", DevPath);
	//Get the End Device Path Node
	while (!IsDevicePathEnd(DevicePath))
	{
		DevicePathMask = (CONST HARDDRIVE_DEVICE_PATH *)DevicePath;
		DevicePath = NextDevicePathNode(DevicePath);
	}
	// DevPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
	// Print(L"DevPath: %s\n", DevPath);
	if (DevicePathMask->Header.Type != MEDIA_DEVICE_PATH)
	{
		continue;
		return EFI_DEVICE_ERROR;
	}

	//Extract Device Path Protocol useful data
	SignatureType = DevicePathMask->SignatureType;
	PartitionNumber = DevicePathMask->PartitionNumber;
	PartitionStart = DevicePathMask->PartitionStart;
	PartitionSize = DevicePathMask->PartitionSize;
	MBRType = DevicePathMask->MBRType;


	//Extract Partition Info Protocol useful data
	Type = PartitionInfo->Type;

	if (DevicePathMask->SignatureType = 2 && DevicePathMask->Type == PartitionInfo->Type)
	{
		//PartitionTypeGUID = PartitionInfo->Info->Gpt->PartitionTypeGUID;
		//UniquePartitionGUID = PartitionInfo->Info->Gpt->UniquePartitionGUID;
		StartingLBA = PartitionInfo->Info->Gpt->StartingLBA;
		EndingLBA = PartitionInfo->Info->Gpt->EndingLBA;
		SizeInLBA = EndingLBA - StartingLBA + 1;
	}

	if (DevicePathMask->SignatureType = 1 && DevicePathMask->Type == PartitionInfo->Type)
	{
		StartingLBA = (EFI_LBA)PartitionInfo->Info->Mbr->StartingLBA;
		SizeInLBA = (EFI_LBA)PartitionInfo->Info->Mbr->SizeInLBA;
		EndingLBA = StartingLBA + SizeInLBA - 1;
	}

	return EFI_NOT_FOUND;
}

EFI_STATUS InitializeDiskIoProtocol(IN EFI_HANDLE ImageHandle)
{
  EFI_DISK_IO_PROTOCOL         *DiskIo;
  EFI_DISK_IO2_PROTOCOL        *DiskIo2;
  EFI_BLOCK_IO_PROTOCOL        *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL       *BlockIo2;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
	CHAR16                       *StrPath;
	UINTN                        DiskDeviceIndex;

	gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, &CurrentImage);
	//Print(L"%0X\n", CurrentImage);
	UINTN BuffCount = 0;
	EFI_HANDLE *HandleBuffer = NULL;
	EFI_STATUS STATUS;
	EFI_STATUS DISKSTATUS;
	// CONST CHAR16 *CurrentName;
	// Locate all devices that support Disk I/O Protocol.
	DISKSTATUS = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BuffCount, &HandleBuffer);
	Print(L"DISKSTATUS:%r\n", DISKSTATUS);
	Print(L"\n");
	if (DISKSTATUS == EFI_SUCCESS)
	{
		DiskDevices = AllocateZeroPool(sizeof(DISK_DEVICE_OBJECT) * BuffCount);
		if (DiskDevices)
		{
			NumberOfDiskDevices = BuffCount;
			for (DiskDeviceIndex = 0; DiskDeviceIndex < BuffCount; DiskDeviceIndex++)
			{
				DiskDevices[DiskDeviceIndex]->DevicePath = DevicePathFromHandle(HandleBuffer[DiskDeviceIndex]);
				STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiBlockIoProtocolGuid, &DiskDevices[DiskDeviceIndex]->BlockIo);
				STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiPartitionInfoProtocolGuid, &DiskDevices[DiskDeviceIndex]->PartInfo);
				StrPath = ConvertDevicePathToText(DiskDevices[DiskDeviceIndex]->DevicePath, FALSE, FALSE);
				Print(L"BlockIo: %d\n", i);
				Print(L"StrPath: %s\n", StrPath);
				Print(L"gEfiPartitionInfoProtocolGuid:%r\n", STATUS);
				Print(L"Type:%d\n", DiskDevices[DiskDeviceIndex]->PartInfo->Type);
				if ((DiskDevices[DiskDeviceIndex]->PartInfo->Type) == 1)
				{
					StartingLBA = (UINT32) DiskDevices[DiskDeviceIndex]->PartInfo->Info->Mbr->Partition[DiskDeviceIndex]->StartingLBA;
					StartingLBA1 = DiskDevices[DiskDeviceIndex]->DevicePath->
				}
				//STATUS = gPartitionDriverBinding.Supported(&gPartitionDriverBinding, HandleBuffer[DiskDeviceIndex], NULL);
				if (DiskDevices[DiskDeviceIndex]->BlockIo->Media->MediaPresent && !DiskDevices[DiskDeviceIndex]->BlockIo->Media->LogicalPartition)
				{
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiBlockIoProtocolGuid, &BlockIo);
					//Print(L"BlockIo: %r\n", STATUS);
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiBlockIo2ProtocolGuid, &BlockIo2);
					//Print(L"BlockIo2: %r\n", STATUS);
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiDiskIoProtocolGuid, &DiskIo);
					//Print(L"DiskIo: %r\n", STATUS);
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiDiskIo2ProtocolGuid, &DiskIo2);
					//Print(L"DiskIo2: %r\n", STATUS);
					DevicePath = DiskDevices[DiskDeviceIndex]->DevicePath;
					Print(L"DiskHandle: %p\n", HandleBuffer[DiskDeviceIndex]);
					//StrPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
					//Print(L"StrPath: %s\n", StrPath);
					STATUS = PartitionInstallGptChildHandles (&gPartitionDriverBinding, HandleBuffer[DiskDeviceIndex], DiskIo, DiskIo2, BlockIo, BlockIo2, DevicePath);
					Print (L"PartitionInstallGptChildHandles: %r\n", STATUS);
					STATUS = PartitionInstallMbrChildHandles (&gPartitionDriverBinding, HandleBuffer[DiskDeviceIndex], DiskIo, DiskIo2, BlockIo, BlockIo2, DevicePath);
					Print (L"PartitionInstallMbrChildHandles: %r\n", STATUS);
					//if (STATUS == EFI_SUCCESS)
					//Print(L"Type:%d\n", (DiskDevices[DiskDeviceIndex]->PartInfo)->Type);
					//STATUS = gBS->HandleProtocol(HandleBuffer[DiskDeviceIndex], &gEfiPartitionInfoProtocolGuid, &DiskDevices[DiskDeviceIndex]->PartInfo);
					//Print(L"PartInfo1: %r\n", STATUS);
					Print(L"\n");
				}
				Print(L"\n");
				/*
				if (HandleBuffer[DiskDeviceIndex] == CurrentImage->DeviceHandle)
				{
					CHAR16 *DevPath = ConvertDevicePathToText(DiskDevices[DiskDeviceIndex]->DevicePath, FALSE, FALSE);
					if (DevPath)
					{
						// CurrentName = gEfiShellProtocol->GetMapFromDevicePath(&DiskDevices[DiskDeviceIndex]->DevicePath);
						// CHAR16 *MapName = StrnCatGrow(&MapName, 0, CurrentName, 0);
						// Print(L"Image was loaded from map: %s, Disk Device: %s\r\n", MapName, DevPath);
						Print(L"Image was loaded from: %s\r\n", DevPath);
						FreePool(DevPath);
						// FreePool(MapName);
					}
				}
				*/
			}
		}
		else
		{
			DISKSTATUS = EFI_OUT_OF_RESOURCES;
			Print(L"Failed to build list of Disk Devices!\r\n");
		}
		FreePool(HandleBuffer);
	}
	else
	{
		Print(L"Failed to locate Disk I/O handles! STATUS=%r\n", DISKSTATUS);
	}
	return DISKSTATUS;
}

EFI_STATUS EFIAPI UefiDiskAccessMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	UINT16 RevHi = (UINT16)(SystemTable->Hdr.Revision >> 16);
	UINT16 RevLo = (UINT16)(SystemTable->Hdr.Revision & 0xFFFF);
	//SetConsoleModeToMaximumRows();
	EnablePageBreak();
	SetConsoleMode();
	Print(L"UefiDiskAccess Demo - Simple Demo of Accessing Disks in UEFI\r\n");
	Print(L"Powered by zero.tangptr@gmail.com, Copyright Zero Tang, 2021, All Rights Reserved.\r\n");
	Print(L"UEFI Firmware Vendor: %s Revision: %d.%d\n", SystemTable->FirmwareVendor, RevHi, RevLo);
	Print(L"DiskIoProtocol: %r\n", STATUS);
	//STATUS = InitializePartition(ImageHandle, SystemTable);
	STATUS = InitializeDiskIoProtocol(ImageHandle);
	if (STATUS == EFI_SUCCESS)
	{
		EnumAllDiskPartitions();
		FreePool(*DiskDevices);
		FreePool(DiskDevices);
	}
	Print(L"Press any key on the keyboard to continue...\r\n");
	BlockUntilAnyKeyStroke();
	return STATUS;
}