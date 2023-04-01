/** @file

  Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (c) 2022 The LineageOS Project

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ANDROID_LAUNCHER_H_
#define __ANDROID_LAUNCHER_H_

#define MAX_BOOTCONFIG_CONTENT_SIZE     512
#define MAX_CBOOTARG_SIZE               256
#define DETACHED_SIG_FILE_EXTENSION     L".sig"

#define BOOTMODE_BOOTIMG_STRING   L"bootmode=bootimg"
#define BOOTMODE_RECOVERY_STRING  L"bootmode=recovery"

#define BOOTCHAIN_OVERRIDE_STRING  L"bootchain="

#define MAX_PARTITION_NAME_SIZE  36       // From the UEFI spec for GPT partitions

#define BOOT_FW_VARIABLE_NAME  L"BootChainFwCurrent"
#define BOOT_OS_VARIABLE_NAME  L"BootChainOsCurrent"

#define ROOTFS_BASE_NAME        L"system"
#define BOOTIMG_BASE_NAME       L"boot"
#define BOOTIMG_DTB_BASE_NAME   L"kernel-dtb"
#define RECOVERY_BASE_NAME      L"recovery"
#define RECOVERY_DTB_BASE_NAME  L"recovery-dtb"

#define SCRATCH0_RECOVERY_BIT_FIELD 31

#endif /* __ANDROID_LAUNCHER_H_ */
