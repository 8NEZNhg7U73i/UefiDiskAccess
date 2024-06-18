/*
  UefiDiskAccess - Simple demo of Accessing Disk I/O

  Copyright 2021 Zero Tang. All rights reserved.

  This program is distributed in the hope that it will be useful, but
  without any warranty (no matter implied warranty or merchantability
  or fitness for a particular purpose, etc.).

  File Location: /src/efimain.h
*/

#include <Uefi.h>
#include <Guid/Gpt.h>
#include <Guid/GlobalVariable.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathUtilities.h>
#include <Protocol/HiiFont.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/BlockIo.h>
#include <Protocol/PartitionInfo.h>

#define LimitKiB	(1<<10)
#define LimitMiB	(1<<20)
#define LimitGiB	(1<<30)

typedef struct _DISK_DEVICE_OBJECT
{
	EFI_DEVICE_PATH_PROTOCOL* DevicePath;
	EFI_BLOCK_IO_PROTOCOL* BlockIo;
  EFI_PARTITION_INFO_PROTOCOL* PartInfo;
}DISK_DEVICE_OBJECT;

#define EFI_PART_TYPE_BASIC_DATA_PART_GUID                                         \
  {                                                                                \
    0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } \
  }

#define EFI_PART_TYPE_MS_RESERVED_PART_GUID                                        \
  {                                                                                \
    0xE3C9E316, 0x0B5C, 0x4DB8, { 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE } \
  }

#define EFI_PART_TYPE_MS_RECOVERY_PART_GUID                                        \
  {                                                                                \
    0xDE94BBA4, 0x06D1, 0x06D1, { 0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC } \
  }

EFI_STATUS EFIAPI UefiBootServicesTableLibConstructor(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable);
EFI_STATUS EFIAPI UefiRuntimeServicesTableLibConstructor(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable);
EFI_STATUS EFIAPI UefiLibConstructor(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable);
EFI_STATUS EFIAPI DevicePathLibConstructor(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable);

EFI_LOADED_IMAGE_PROTOCOL *CurrentImage=NULL;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *StdIn=NULL;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdOut=NULL;
/*
EFI_GUID gEfiSimpleFileSystemProtocolGuid=EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputProtocolGuid=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiUgaDrawProtocolGuid=EFI_UGA_DRAW_PROTOCOL_GUID;
EFI_GUID gEfiHiiFontProtocolGuid=EFI_HII_FONT_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextOutProtocolGuid=EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathProtocolGuid=EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathUtilitiesProtocolGuid=EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathToTextProtocolGuid=EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathFromTextProtocolGuid=EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID;
EFI_GUID gEfiGlobalVariableGuid=EFI_GLOBAL_VARIABLE;
*/
EFI_GUID gEfiBlockIoProtocolGuid=EFI_BLOCK_IO_PROTOCOL_GUID;
EFI_GUID gEfiPartTypeUnusedGuid=EFI_PART_TYPE_UNUSED_GUID;
EFI_GUID gEfiPartTypeSystemPartGuid=EFI_PART_TYPE_EFI_SYSTEM_PART_GUID;
EFI_GUID gEfiPartTypeMsReservedPartGuid = EFI_PART_TYPE_MS_RESERVED_PART_GUID;
EFI_GUID gEfiPartTypeBasicDataPartGuid = EFI_PART_TYPE_BASIC_DATA_PART_GUID;
EFI_GUID gEfiPartTypeMsRecoveryPartGuid = EFI_PART_TYPE_MS_RECOVERY_PART_GUID;
EFI_GUID gEfiLoadedImageProtocolGuid=EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_STATUS FindGptSignature(CONST EFI_DEVICE_PATH_PROTOCOL* DevicePath, EFI_GUID* GptSignature);

UINTN NumberOfDiskDevices=0;
DISK_DEVICE_OBJECT *DiskDevices=NULL;
DISK_DEVICE_OBJECT *CurrentDiskDevice=NULL;
UINTN NumberOfPartitions=0;
DISK_DEVICE_OBJECT *Partitions=NULL;
DISK_DEVICE_OBJECT *CurrentPartitions=NULL;

extern EFI_BOOT_SERVICES *gBS;
extern EFI_SYSTEM_TABLE *gST;