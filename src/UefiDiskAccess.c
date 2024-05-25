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
		UINTN fi=0;
		gBS->WaitForEvent(1,&StdIn->WaitForKey,&fi);
		StdIn->ReadKeyStroke(StdIn,&InKey);
	}while(InKey.UnicodeChar!=Unicode && Unicode);
	return InKey.UnicodeChar;
}

INTN EfiCompareGuid(EFI_GUID *Guid1,EFI_GUID *Guid2)
{
    if(Guid1->Data1>Guid2->Data1)
        return 1;
    else if(Guid1->Data1<Guid2->Data1)
        return -1;
    if(Guid1->Data2>Guid2->Data2)
        return 1;
    else if(Guid1->Data2<Guid2->Data2)
        return -1;
    if(Guid1->Data3>Guid2->Data3)
        return 1;
    else if(Guid1->Data3<Guid2->Data3)
        return -1;
    for(UINT8 i=0;i<8;i++)
    {
        if(Guid1->Data4[i]>Guid2->Data4[i])
            return 1;
        else if(Guid1->Data4[i]<Guid2->Data4[i])
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

void SetConsoleModeToMaximumRows()
{
	UINTN MaxHgt=0,OptIndex;
	for(UINTN i=0;i<StdOut->Mode->MaxMode;i++)
	{
		UINTN Col,Row;
		EFI_STATUS STATUS=StdOut->QueryMode(StdOut,i,&Col,&Row);
		if(STATUS==EFI_SUCCESS)
		{
			if(Row>MaxHgt)
			{
				OptIndex=i;
				MaxHgt=Row;
			}
		}
	}
	StdOut->SetMode(StdOut,OptIndex);
	StdOut->ClearScreen(StdOut);
	StdOut->SetAttribute(StdOut,EFI_BACKGROUND_BLACK|EFI_LIGHTGRAY);
}

void DisplaySize(IN UINT64 Size,OUT CHAR16 *Buffer,IN UINTN Limit)
{
	if(Size<LimitKiB)
		UnicodeSPrint(Buffer,Limit,L"%u Bytes",Size);
	else if(Size>=LimitKiB && Size<LimitMiB)
		UnicodeSPrint(Buffer,Limit,L"%u KiB",Size>>10);
	else if(Size>=LimitMiB && Size<LimitGiB)
		UnicodeSPrint(Buffer,Limit,L"%u MiB",Size>>20);
	else
		UnicodeSPrint(Buffer,Limit,L"%u GiB",Size>>30);
}

EFI_STATUS EnumDiskPartitions(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol)
{
	EFI_STATUS STATUS=EFI_DEVICE_ERROR;
	if(!BlockIoProtocol->Media->LogicalPartition)
	{
		MASTER_BOOT_RECORD MBRContent;
		STATUS=BlockIoProtocol->ReadBlocks(BlockIoProtocol,BlockIoProtocol->Media->MediaId,0,sizeof(MASTER_BOOT_RECORD),&MBRContent);
		if(STATUS==EFI_SUCCESS)
		{
			if(MBRContent.Signature!=MBR_SIGNATURE)
				Print(L"Invalid MBR Signature! MBR might be corrupted!\r\n");
			for(UINT8 i=0;i<MAX_MBR_PARTITIONS;i++)
			{
				MBR_PARTITION_RECORD *Part=&MBRContent.Partition[i];
				if(Part->OSIndicator)
				{
					UINT32 StartLBA=*(UINT32*)Part->StartingLBA;
					UINT32 SizeInLBA=*(UINT32*)Part->SizeInLBA;
					UINT32 EndLBA=(StartLBA + SizeInLBA - 1);
					CHAR16 ScaledStart[32],ScaledEnd[32],ScaledSize[32];
					DisplaySize(__emulu(StartLBA,BlockIoProtocol->Media->BlockSize),ScaledStart,sizeof(ScaledStart));
					DisplaySize(__emulu(EndLBA,BlockIoProtocol->Media->BlockSize),ScaledEnd,sizeof(ScaledEnd));
					DisplaySize(__emulu(SizeInLBA,BlockIoProtocol->Media->BlockSize),ScaledSize,sizeof(ScaledSize));
					if(SizeInLBA==0xFFFFFFFF)
					{
						Print(L"MBR Part %d: OS Type: 0x%02X Start: %s End: %s Size: Over 2TiB\n",i,Part->OSIndicator,ScaledStart,ScaledEnd);
					}
					else
					{
						Print(L"MBR Part %d: OS Type: 0x%02X StartLBA: %u EndLBA: %u Size: %s LBASize: %u\n",i,Part->OSIndicator,StartLBA,EndLBA,ScaledSize,SizeInLBA);
					}
					if(Part->OSIndicator==PMBR_GPT_PARTITION || Part->OSIndicator==EFI_PARTITION)
					{
						EFI_PARTITION_TABLE_HEADER *GptHeader=AllocatePool(BlockIoProtocol->Media->BlockSize);
						if(GptHeader)
						{
							STATUS=BlockIoProtocol->ReadBlocks(BlockIoProtocol,BlockIoProtocol->Media->MediaId,StartLBA,BlockIoProtocol->Media->BlockSize,GptHeader);
							if(STATUS==EFI_SUCCESS)
							{
								if(GptHeader->Header.Signature!=EFI_PTAB_HEADER_ID)
									Print(L"Improper GPT Header Signature!");
								else
								{
									UINT32 PartitionEntrySize=GptHeader->SizeOfPartitionEntry*GptHeader->NumberOfPartitionEntries;
									VOID* PartitionEntries=AllocatePool(PartitionEntrySize);
									Print(L"Disk GUID: {%g} Part Array LBA: %u Number of Partitions: %u\n",&GptHeader->DiskGUID,GptHeader->PartitionEntryLBA,GptHeader->NumberOfPartitionEntries);
									if(PartitionEntries)
									{
										STATUS=BlockIoProtocol->ReadBlocks(BlockIoProtocol,BlockIoProtocol->Media->MediaId,GptHeader->PartitionEntryLBA,PartitionEntrySize,PartitionEntries);
										if(STATUS==EFI_SUCCESS)
										{
											for(UINT32 j=0;j<GptHeader->NumberOfPartitionEntries;j++)
											{
												EFI_PARTITION_ENTRY *PartitionEntry=(EFI_PARTITION_ENTRY*)((UINTN)PartitionEntries+j*GptHeader->SizeOfPartitionEntry);
												if(EfiCompareGuid(&PartitionEntry->PartitionTypeGUID,&gEfiPartTypeUnusedGuid))
												{
													DisplaySize(MultU64x32(PartitionEntry->StartingLBA,BlockIoProtocol->Media->BlockSize),ScaledStart,sizeof(ScaledStart));
													DisplaySize(MultU64x32(PartitionEntry->EndingLBA,BlockIoProtocol->Media->BlockSize),ScaledEnd,sizeof(ScaledEnd));
													DisplaySize(MultU64x32(PartitionEntry->EndingLBA-PartitionEntry->StartingLBA+1,BlockIoProtocol->Media->BlockSize),ScaledSize,sizeof(ScaledSize));
													for(UINT32 k=0;k<NumberOfDiskDevices;k++)
													{
														if (DiskDevices[k].DevicePath)
														{
															STATUS=FindGptSignature(DiskDevices[k].DevicePath, &PartitionEntry->UniquePartitionGUID);
															if (STATUS==EFI_SUCCESS)
															{
																Print(L"GPT Part %u, Block Device %u : StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n",j,k,PartitionEntry->StartingLBA,PartitionEntry->EndingLBA,PartitionEntry->EndingLBA-PartitionEntry->StartingLBA+1,ScaledSize);
																break;
															}
															if (k==NumberOfDiskDevices-1)
															{
																Print(L"GPT Part %u : StartLBA: %u EndLBA: %u LBASize: %u Size: %s\n", j, PartitionEntry->StartingLBA, PartitionEntry->EndingLBA, PartitionEntry->EndingLBA - PartitionEntry->StartingLBA + 1, ScaledSize);
															}
														}
													}

													if (EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeSystemPartGuid))
													{
														Print(L"Part Type : efi\n");
													}
													else if (EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeMsReservedPartGuid))
													{
														Print(L"Part Type : msr\n");
													}
													else if (EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeBasicDataPartGuid))
													{
														Print(L"Part Type : data\n");
													}
													else if (EfiCompareGuid(&PartitionEntry->PartitionTypeGUID, &gEfiPartTypeMsRecoveryPartGuid))
													{
														Print(L"Part Type : wre\n");
													}
													else
													{
														Print(L"Part Type GUID:    {%g}\n",&PartitionEntry->PartitionTypeGUID);
													}
													Print(L"Unique Part GUID:  {%g}\n",&PartitionEntry->UniquePartitionGUID);
													Print(L"\n");
												}
											}
										}
										FreePool(PartitionEntries);
									}
								}
							}
							else
								Print(L"Failed to read GPT Header! Status=0x%p\n",STATUS);
							FreePool(GptHeader);
						}
					}
				}
			}
		}
	}
	return STATUS;
}

void EnumAllDiskPartitions()
{
	for(UINTN i=0;i<NumberOfDiskDevices;i++)
	{
		// Skip absent media and partition media.
		if(DiskDevices[i].BlockIo->Media->MediaPresent && !DiskDevices[i].BlockIo->Media->LogicalPartition)
		{
		CHAR16 *DiskDevicePath = ConvertDevicePathToText(DiskDevices[i].DevicePath, FALSE, FALSE);
			Print(L"=============================================================================\r\n");
			Print(L"Part Info of Block Device %u Path: %s\n", i, DiskDevicePath);
			FreePool(DiskDevicePath);
			Print(L"Last LBA: 0x%llX (%u).\n", DiskDevices[i].BlockIo->Media->LastBlock, DiskDevices[i].BlockIo->Media->LastBlock);
			EnumDiskPartitions(DiskDevices[i].BlockIo);
		}
	}
	Print(L"=============================================================================\r\n");
}

EFI_STATUS FindGptSignature(CONST EFI_DEVICE_PATH_PROTOCOL* DevicePath, EFI_GUID* GptSignature)
{
	CONST HARDDRIVE_DEVICE_PATH *DevicePathMask;
	if (!DevicePath || !GptSignature)
	{
		return EFI_INVALID_PARAMETER;
	}
	while (!IsDevicePathEnd(DevicePath))
	{
		DevicePathMask = (CONST HARDDRIVE_DEVICE_PATH*)DevicePath;
		DevicePath = NextDevicePathNode(DevicePath);
		if (DevicePathMask->Header.Type != MEDIA_DEVICE_PATH)
		{
			continue;
		}
		// Check if the device path describes a GPT partition or disk
		if (DevicePathMask->SignatureType != 2)
		{
			continue;
		}
		if(!CompareMem (GptSignature, DevicePathMask->Signature, sizeof(EFI_GUID))){
			return EFI_SUCCESS;
		}
	}
	return EFI_NOT_FOUND;
}

EFI_STATUS InitializeDiskIoProtocol()
{
	UINTN BuffCount=0;
	EFI_HANDLE *HandleBuffer=NULL;
	CONST CHAR16* CurrentName;
	// Locate all devices that support Disk I/O Protocol.
	EFI_STATUS STATUS=gBS->LocateHandleBuffer(ByProtocol,&gEfiBlockIoProtocolGuid,NULL,&BuffCount,&HandleBuffer);
	if(STATUS==EFI_SUCCESS)
	{
		DiskDevices=AllocateZeroPool(sizeof(DISK_DEVICE_OBJECT)*BuffCount);
		if(DiskDevices)
		{
			NumberOfDiskDevices=BuffCount;
			for(UINTN i=0;i<BuffCount;i++)
			{
				DiskDevices[i].DevicePath=DevicePathFromHandle(HandleBuffer[i]);
				gBS->HandleProtocol(HandleBuffer[i],&gEfiBlockIoProtocolGuid,&DiskDevices[i].BlockIo);
				if(HandleBuffer[i]==CurrentImage->DeviceHandle)
				{
					CHAR16* DevPath=ConvertDevicePathToText(DiskDevices[i].DevicePath,FALSE,FALSE);
					if(DevPath)
					{
						CurrentName=gEfiShellProtocol->GetMapFromDevicePath(&DiskDevices[i].DevicePath);
						CHAR16* MapName=StrnCatGrow(&MapName, 0, CurrentName,0);
						Print(L"Image was loaded from map: %s, Disk Device: %s\r\n", MapName, DevPath);
						FreePool(DevPath);
						FreePool(MapName);
					}
				}
			}
		}
		else
		{
			STATUS=EFI_OUT_OF_RESOURCES;
			Print(L"Failed to build list of Disk Devices!\r\n");
		}
		FreePool(HandleBuffer);
	}
	else
		Print(L"Failed to locate Disk I/O handles! Status=0x%p\n",STATUS);
	return STATUS;
}

EFI_STATUS EFIAPI EfiInitialize(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable)
{
	UefiBootServicesTableLibConstructor(ImageHandle,SystemTable);
	UefiRuntimeServicesTableLibConstructor(ImageHandle,SystemTable);
	UefiLibConstructor(ImageHandle,SystemTable);
	DevicePathLibConstructor(ImageHandle,SystemTable);
	StdIn=SystemTable->ConIn;
	StdOut=SystemTable->ConOut;
	return gBS->HandleProtocol(ImageHandle,&gEfiLoadedImageProtocolGuid,&CurrentImage);
}

EFI_STATUS EFIAPI UefiDiskAccessMain(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS STATUS=EfiInitialize(ImageHandle,SystemTable);
	if(STATUS==EFI_SUCCESS)
	{
		UINT16 RevHi=(UINT16)(SystemTable->Hdr.Revision>>16);
		UINT16 RevLo=(UINT16)(SystemTable->Hdr.Revision&0xFFFF);
		SetConsoleModeToMaximumRows();
		EnablePageBreak();
		Print(L"UefiDiskAccess Demo - Simple Demo of Accessing Disks in UEFI\r\n");
		Print(L"Powered by zero.tangptr@gmail.com, Copyright Zero Tang, 2021, All Rights Reserved.\r\n");
		Print(L"UEFI Firmware Vendor: %s Revision: %d.%d\n",SystemTable->FirmwareVendor,RevHi,RevLo);
		STATUS=InitializeDiskIoProtocol();
		if(STATUS==EFI_SUCCESS)
		{
			EnumAllDiskPartitions();
			FreePool(DiskDevices);
		}
		//Print(L"Press Enter key to continue...\r\n");
		//BlockUntilKeyStroke(L'\r');
	}
	return STATUS;
}