/** @file
  Rootfs Validation Private Structures.

  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ROOTFSVALIDATIONPRIVATE_H__
#define __ROOTFSVALIDATIONPRIVATE_H__

#define ROOTFS_SLOT_A  0
#define ROOTFS_SLOT_B  1

#define ROOTFS_RETRY_MAX 7

#define ANDROIDLAUNCHER_STATUS_NORMAL     NVIDIA_OS_STATUS_NORMAL
#define ANDROIDLAUNCHER_STATUS_UNBOOTABLE NVIDIA_OS_STATUS_UNBOOTABLE
#define ANDROIDLAUNCHER_STATUS_BOOTING    0x01

#define DELAY_SECOND  1000000

typedef enum {
  RF_STATUS_A,
  RF_STATUS_B,

  RF_RETRY_A,
  RF_RETRY_B,

  RF_FW_NEXT,
  RF_BC_STATUS,

  RF_VARIABLE_INDEX_MAX
} RF_VARIABLE_INDEX;

typedef struct {
  UINT32    Value;
  UINT32    UpdateFlag; // 1 - update, 0 - not update
} RF_VARIABLE;

typedef struct {
  RF_VARIABLE    RootfsVar[RF_VARIABLE_INDEX_MAX];
  UINT32         CurrentSlot;
} L4T_RF_AB_PARAM;

typedef struct {
  CHAR16      *Name;
  UINT32      Attributes;
  UINT8       Bytes;
  EFI_GUID    *Guid;
} RF_AB_VARIABLE;

typedef struct {
  UINT32    BootMode;
  UINT32    BootChain;
} L4T_BOOT_PARAMS;

/**
  Validate rootfs A/B status and update BootMode and BootChain accordingly, basic flow:
  If there is no rootfs B,
     (1) boot to rootfs A if retry count of rootfs A is not 0;
     (2) boot to recovery if rtry count of rootfs A is 0.
  If there is rootfs B,
     (1) boot to current rootfs slot if the retry count of current slot is not 0;
     (2) switch to non-current rootfs slot if the retry count of current slot is 0
         and non-current rootfs is bootable
     (3) boot to recovery if both rootfs slots are invalid.

  @param[out] BootParams      The current rootfs boot parameters

  @retval EFI_SUCCESS    The operation completed successfully.

**/
EFI_STATUS
EFIAPI
ValidateRootfsStatus (
  OUT L4T_BOOT_PARAMS  *BootParams
  );

#endif
