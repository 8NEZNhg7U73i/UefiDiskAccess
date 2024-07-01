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

EFI_STATUS FindMbrBlockDevice(IN MBR_PARTITION_RECORD *Mbr, IN UINTN MbrPartIndex, OUT EFI_DEVICE_PATH_PROTOCOL *DevicePath, OUT UINTN DiskIndex)
{
	CONST HARDDRIVE_DEVICE_PATH *DevicePathMask;
	EFI_STATUS STATUS;
	//EFI_LBA StartingLBA;
	//EFI_LBA	EndingLBA;
	for (DiskIndex = 0 ; DiskIndex < NumberOfDiskDevices; DiskIndex++)
	{
		STATUS = CompareMem(&DiskDevices[DiskIndex]->PartInfo->Info.Mbr, Mbr, sizeof(MBR_PARTITION_RECORD));
		if (STATUS == EFI_SUCCESS)
		{
			while (!IsDevicePathEnd(DiskDevices[DiskIndex]->DevicePath))
			{
				DevicePathMask = (CONST HARDDRIVE_DEVICE_PATH *)DevicePath;
				DevicePath = NextDevicePathNode(DevicePath);
			}
			if (DevicePathMask->PartitionNumber == MbrPartIndex)
			{
				DevicePath = DiskDevices[DiskIndex]->DevicePath;
				return EFI_SUCCESS;
			}
		}
	}
	return EFI_DEVICE_ERROR;
}

EFI_STATUS FindGptBlockDevice(IN EFI_PARTITION_ENTRY *Gpt, IN UINTN GptPartIndex, OUT EFI_DEVICE_PATH_PROTOCOL *DevicePath, OUT UINTN DiskIndex)
{
	CONST HARDDRIVE_DEVICE_PATH *DevicePathMask;
	EFI_STATUS STATUS;
	//EFI_LBA StartingLBA;
	//EFI_LBA EndingLBA;
	for (DiskIndex = 0 ; DiskIndex < NumberOfDiskDevices; DiskIndex++)
	{
		STATUS = CompareMem(&DiskDevices[DiskIndex]->PartInfo->Info.Gpt, Gpt, sizeof(EFI_PARTITION_ENTRY));
		if (STATUS == EFI_SUCCESS)
		{
			while (!IsDevicePathEnd(DiskDevices[DiskIndex]->DevicePath))
			{
				DevicePathMask = (CONST HARDDRIVE_DEVICE_PATH *)DevicePath;
				DevicePath = NextDevicePathNode(DevicePath);
			}
			if (DevicePathMask->PartitionNumber == GptPartIndex)
			{
				DevicePath = DiskDevices[DiskIndex]->DevicePath;
				return EFI_SUCCESS;
			}
		}
	}
	return EFI_DEVICE_ERROR;
}

EFI_STATUS EnumMbrDisk(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol, OUT EFI_LBA MyLBA)
{
	EFI_STATUS STATUS;
	MASTER_BOOT_RECORD *MBRContent=NULL;
	MBR_PARTITION_RECORD *MbrPart=NULL;
	UINT32 StartingLBA, SizeInLBA, EndingLBA;
	CHAR16 ScaledStart[32], ScaledEnd[32], ScaledSize[32];
	EFI_DEVICE_PATH_PROTOCOL *DevicePath=NULL;
	UINTN MbrPartIndex=0;
	UINTN DiskIndex=0;

	MyLBA = 0;
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
				StartingLBA = *(UINT32 *)MbrPart->StartingLBA;
				SizeInLBA = *(UINT32 *)MbrPart->SizeInLBA;
				EndingLBA = (StartingLBA + SizeInLBA - 1);
				if (MbrPart->OSIndicator == PMBR_GPT_PARTITION || MbrPart->OSIndicator == EFI_PARTITION)
				{
					MyLBA = StartingLBA;
				}
				else
				{
					DisplaySize(__emulu(StartingLBA, BlockIoProtocol->Media->BlockSize), ScaledStart, sizeof(ScaledStart));
					DisplaySize(__emulu(EndingLBA, BlockIoProtocol->Media->BlockSize), ScaledEnd, sizeof(ScaledEnd));
					DisplaySize(__emulu(SizeInLBA, BlockIoProtocol->Media->BlockSize), ScaledSize, sizeof(ScaledSize));
					STATUS = FindMbrBlockDevice(MbrPart, MbrPartIndex, DevicePath, DiskIndex);
					if (STATUS == EFI_SUCCESS)
					{
						if (SizeInLBA == 0xFFFFFFFF)
						{
							Print(L"MBR Part %d, Block Device %d : StartLBA: %u EndLBA: %s OS Type: 0x%02X Size: Over 2TiB\n", MbrPartIndex, DiskIndex , StartingLBA, EndingLBA, MbrPart->OSIndicator);
						}
						else
						{
							Print(L"MBR Part %d, Block Device %d : StartLBA: %u EndLBA: %u LBASize: %u OS Type: 0x%02X Size: %s\n", MbrPartIndex, DiskIndex, StartingLBA, EndingLBA, SizeInLBA, ScaledSize, MbrPart->OSIndicator);
						}
					}
					else {
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
	}
	else
	{
		Print(L"Failed to read MBR Header! STATUS=%r\n", STATUS);
		return EFI_OUT_OF_RESOURCES;
	}
	FreePool(MBRContent);
	return EFI_SUCCESS;
}

EFI_STATUS EnumGptDisk(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol, IN UINTN MyLBA)
{
	EFI_STATUS STATUS;
	EFI_PARTITION_TABLE_HEADER *PartitionHeader=NULL;
	EFI_PARTITION_ENTRY *PartitionEntry=NULL;
	UINT32 PartitionEntrySize;
	VOID *PartitionEntries;
	UINTN PartitionIndex=0;
	UINTN DiskIndex=0;
	CHAR16 ScaledStart[32], ScaledEnd[32], ScaledSize[32];
	EFI_DEVICE_PATH_PROTOCOL *DevicePath=NULL;

	STATUS = BlockIoProtocol->ReadBlocks(BlockIoProtocol, BlockIoProtocol->Media->MediaId, MyLBA, BlockIoProtocol->Media->BlockSize, PartitionHeader);
	if (STATUS == EFI_SUCCESS)
	{
		if (PartitionHeader->Header.Signature != EFI_PTAB_HEADER_ID)
		{
			Print(L"Improper GPT Header Signature!");
			return EFI_DEVICE_ERROR;
		}
		else
		{
			Print(L"GPT Header Detected! First usable LBA: %u. Last usable LBA: %u.\n", PartitionHeader->FirstUsableLBA, PartitionHeader->LastUsableLBA);
			PartitionEntrySize = PartitionHeader->SizeOfPartitionEntry * PartitionHeader->NumberOfPartitionEntries;
			PartitionEntries = AllocatePool(PartitionEntrySize);
			Print(L"Disk GUID: {%g} Max number of partitions: %u\n", &PartitionHeader->DiskGUID, PartitionHeader->NumberOfPartitionEntries);
			if (PartitionEntries)
			{
				STATUS = BlockIoProtocol->ReadBlocks(BlockIoProtocol, BlockIoProtocol->Media->MediaId, PartitionHeader->PartitionEntryLBA, PartitionEntrySize, PartitionEntries);
				if (STATUS == EFI_SUCCESS)
				{
					for (PartitionIndex = 0; PartitionIndex < PartitionHeader->NumberOfPartitionEntries; PartitionIndex++)
					{
						PartitionEntry = (EFI_PARTITION_ENTRY *)((UINTN)PartitionEntries + PartitionIndex * PartitionHeader->SizeOfPartitionEntry);
						if (EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid))
						{
							DisplaySize(MultU64x32(PartitionEntry->StartingLBA, BlockIoProtocol->Media->BlockSize), ScaledStart, sizeof(ScaledStart));
							DisplaySize(MultU64x32(PartitionEntry->EndingLBA, BlockIoProtocol->Media->BlockSize), ScaledEnd, sizeof(ScaledEnd));
							DisplaySize(MultU64x32(PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, BlockIoProtocol->Media->BlockSize), ScaledSize, sizeof(ScaledSize));
							STATUS = FindGptBlockDevice(PartitionEntry, PartitionIndex, DevicePath, DiskIndex);
							if (STATUS == EFI_SUCCESS)
							{
								Print(L"GPT Part %u, Block Device %u : StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n", PartitionIndex, DiskIndex, PartitionEntry->StartingLBA, PartitionEntry->EndingLBA, PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, ScaledSize);
							}
							else
							{
								Print(L"GPT Part %u : StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n", PartitionIndex , PartitionEntry->StartingLBA, PartitionEntry->EndingLBA, PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, ScaledSize);
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
			}
			FreePool(PartitionEntries);
		}
	}
	else
	{
		Print(L"Failed to read GPT Header! STATUS=%r\n", STATUS);
		return EFI_OUT_OF_RESOURCES;
	}
	FreePool(PartitionHeader);
	return EFI_SUCCESS;
}



EFI_STATUS EnumDiskPartitions(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol)
{
	EFI_STATUS STATUS;
	EFI_LBA MyLBA=0;
	if (!BlockIoProtocol->Media->LogicalPartition)
	{
		STATUS = EnumMbrDisk(BlockIoProtocol, MyLBA);
		if (!MyLBA)
		{
			STATUS = EnumGptDisk(BlockIoProtocol, MyLBA);
		}
	}
	return STATUS;
}

void EnumAllDiskPartitions()
{
	UINTN DiskIndex;
	for (DiskIndex = 0; DiskIndex < NumberOfDiskDevices; DiskIndex++)
	{
		// Skip absent media and partition media.
		Print(L"LogicalPartition :%d\n", DiskDevices[DiskIndex]->BlockIo->Media->LogicalPartition);
		if (DiskDevices[DiskIndex]->BlockIo->Media->MediaPresent && !DiskDevices[DiskIndex]->BlockIo->Media->LogicalPartition)
		{
			CHAR16 *DiskDevicePath = ConvertDevicePathToText(DiskDevices[DiskIndex]->DevicePath, FALSE, FALSE);
			Print(L"=============================================================================\r\n");
			Print(L"Part Info of Block Device %u Path: %s\n", DiskIndex, DiskDevicePath);
			FreePool(DiskDevicePath);
			Print(L"Disk Last LBA: %u.\n", DiskDevices[DiskIndex]->BlockIo->Media->LastBlock);
			EnumDiskPartitions(DiskDevices[DiskIndex]->BlockIo);
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

//EFI_STATUS DevicePathConvert(CONST IN EFI_DEVICE_PATH_PROTOCOL *DevicePath, IN EFI_BLOCK_IO_PROTOCOL *BlockIo, IN EFI_PARTITION_INFO_PROTOCOL *PartitionInfo)
EFI_STATUS DevicePathConvert(IN DISK_DEVICE_OBJECT *DiskDevice)
{
	CONST HARDDRIVE_DEVICE_PATH *DevicePathMask;
	//EFI_LBA LastBlock;
	EFI_LBA StartingLBA;
	EFI_LBA EndingLBA;
	EFI_LBA SizeInLBA;
	EFI_GUID PartitionTypeGUID;
	EFI_GUID UniquePartitionGUID;
	EFI_LBA PartitionStart;
	EFI_LBA PartitionSize;
	UINT32 PartitionNumber;
	UINT8 SignatureType;
	BOOLEAN IsDisk;
	EFI_DEVICE_PATH_PROTOCOL *DevicePath = DiskDevice->DevicePath;
	//EFI_BLOCK_IO_PROTOCOL *BlockIO = DiskDevice->BlockIo;
	EFI_PARTITION_INFO_PROTOCOL *PartitionInfo = DiskDevice->PartInfo;
	UINTN MBRType;
	UINTN Type;

	if (!DiskDevice)
	{
		return EFI_INVALID_PARAMETER;
	}
	//CHAR16 *DevPath;

	
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
		return EFI_DEVICE_ERROR;
	}

	if (PartitionInfo && DevicePathMask->SignatureType)
	{
		IsDisk = TRUE;
		return EFI_SUCCESS;
	}
	else
	{
		IsDisk = FALSE;
	}

	//Extract Device Path Protocol useful data
	SignatureType = DevicePathMask->SignatureType;
	PartitionNumber = DevicePathMask->PartitionNumber;
	PartitionStart = DevicePathMask->PartitionStart;
	PartitionSize = DevicePathMask->PartitionSize;
	MBRType = DevicePathMask->MBRType;


	//Extract Partition Info Protocol useful data
	Type = PartitionInfo->Type;

	if (DevicePathMask->SignatureType == 2 && DevicePathMask->MBRType == PartitionInfo->Type)
	{
		PartitionTypeGUID = PartitionInfo->Info.Gpt.PartitionTypeGUID;
		UniquePartitionGUID = PartitionInfo->Info.Gpt.UniquePartitionGUID;
		StartingLBA = PartitionInfo->Info.Gpt.StartingLBA;
		EndingLBA = PartitionInfo->Info.Gpt.EndingLBA;
		SizeInLBA = EndingLBA - StartingLBA + 1;
	}

	if (DevicePathMask->SignatureType == 1 && DevicePathMask->MBRType == PartitionInfo->Type)
	{
		StartingLBA = (EFI_LBA)PartitionInfo->Info.Mbr.StartingLBA;
		SizeInLBA = (EFI_LBA)PartitionInfo->Info.Mbr.SizeInLBA;
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
	UINTN                        DiskIndex;

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
			Print(L"NumberOfDiskDevices: %d\n", NumberOfDiskDevices);
			for (DiskIndex = 0; DiskIndex < BuffCount; DiskIndex++)
			{
				DiskDevices[DiskIndex]->DevicePath = DevicePathFromHandle(HandleBuffer[DiskIndex]);
				STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiBlockIoProtocolGuid, &DiskDevices[DiskIndex]->BlockIo);
				STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiPartitionInfoProtocolGuid, &DiskDevices[DiskIndex]->PartInfo);
				StrPath = ConvertDevicePathToText(DiskDevices[DiskIndex]->DevicePath, FALSE, FALSE);
				Print(L"BlockIo: %d\n", DiskIndex);
				Print(L"StrPath: %s\n", StrPath);
				Print(L"gEfiPartitionInfoProtocolGuid:%r\n", STATUS);
				Print(L"Type:%d\n", DiskDevices[DiskIndex]->PartInfo->Type);
				Print(L"LogicalPartition :%d\n", DiskDevices[DiskIndex]->BlockIo->Media->LogicalPartition);
				//Print(L"DiskIoProtocol: %r\n", STATUS);
				//STATUS = gPartitionDriverBinding.Supported(&gPartitionDriverBinding, HandleBuffer[DiskIndex], NULL);
				if (!DiskDevices[DiskIndex]->BlockIo->Media->MediaPresent && !DiskDevices[DiskIndex]->BlockIo->Media->LogicalPartition)
				{
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiBlockIoProtocolGuid, &BlockIo);
					//Print(L"BlockIo: %r\n", STATUS);
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiBlockIo2ProtocolGuid, &BlockIo2);
					//Print(L"BlockIo2: %r\n", STATUS);
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiDiskIoProtocolGuid, &DiskIo);
					//Print(L"DiskIo: %r\n", STATUS);
					STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiDiskIo2ProtocolGuid, &DiskIo2);
					//Print(L"DiskIo2: %r\n", STATUS);
					DevicePath = DiskDevices[DiskIndex]->DevicePath;
					Print(L"DiskHandle: %p\n", HandleBuffer[DiskIndex]);
					//StrPath = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
					//Print(L"StrPath: %s\n", StrPath);
					STATUS = PartitionInstallGptChildHandles (&gPartitionDriverBinding, HandleBuffer[DiskIndex], DiskIo, DiskIo2, BlockIo, BlockIo2, DevicePath);
					Print (L"PartitionInstallGptChildHandles: %r\n", STATUS);
					STATUS = PartitionInstallMbrChildHandles (&gPartitionDriverBinding, HandleBuffer[DiskIndex], DiskIo, DiskIo2, BlockIo, BlockIo2, DevicePath);
					Print (L"PartitionInstallMbrChildHandles: %r\n", STATUS);
					//if (STATUS == EFI_SUCCESS)
					//Print(L"Type:%d\n", (DiskDevices[DiskIndex]->PartInfo)->Type);
					//STATUS = gBS->HandleProtocol(HandleBuffer[DiskIndex], &gEfiPartitionInfoProtocolGuid, DiskDevices[DiskIndex]->PartInfo);
					//Print(L"PartInfo1: %r\n", STATUS);
					Print(L"\n");
				}
				Print(L"\n");

				/*
				if (HandleBuffer[DiskIndex] == CurrentImage->DeviceHandle)
				{
					CHAR16 *DevPath = ConvertDevicePathToText(DiskDevices[DiskIndex]->DevicePath, FALSE, FALSE);
					if (DevPath)
					{
						// CurrentName = gEfiShellProtocol->GetMapFromDevicePath(DiskDevices[DiskIndex]->DevicePath);
						// CHAR16 *MapName = StrnCatGrow(&MapName, 0, CurrentName, 0);
						// Print(L"Image was loaded from map: %s, Disk Device: %s\r\n", MapName, DevPath);
						Print(L"Image was loaded from: %s\r\n", DevPath);
						FreePool(DevPath);
						// FreePool(MapName);
					}
				}
				*/
			}
			for (DiskIndex = 0; DiskIndex < BuffCount; DiskIndex++)
			{
				Print(L"LogicalPartition :%d\n", DiskDevices[DiskIndex]->BlockIo->Media->LogicalPartition);
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
	EFI_STATUS STATUS;
	UINT16 RevHi = (UINT16)(SystemTable->Hdr.Revision >> 16);
	UINT16 RevLo = (UINT16)(SystemTable->Hdr.Revision & 0xFFFF);
	//SetConsoleModeToMaximumRows();
	EnablePageBreak();
	SetConsoleMode();
	Print(L"UefiDiskAccess Demo - Simple Demo of Accessing Disks in UEFI\r\n");
	Print(L"Powered by zero.tangptr@gmail.com, Copyright Zero Tang, 2021, All Rights Reserved.\r\n");
	Print(L"UEFI Firmware Vendor: %s Revision: %d.%d\n", SystemTable->FirmwareVendor, RevHi, RevLo);
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