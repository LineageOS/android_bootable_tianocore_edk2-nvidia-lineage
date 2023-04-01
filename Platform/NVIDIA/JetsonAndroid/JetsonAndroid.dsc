#
#  Copyright (c) 2018-2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#  Copyright (c) 2013-2018, ARM Limited. All rights reserved.
#  Copyright (c) 2023 The LineageOS Project
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME               = JetsonAndroid
  PLATFORM_GUID               = 349f0ff8-c3ff-4c86-9f6b-6b7db353f571
  OUTPUT_DIRECTORY            = Build/JetsonAndroid
  FLASH_DEFINITION            = Platform/NVIDIA/JetsonAndroid/JetsonAndroid.fdf

[SkuIds]
  0|DEFAULT
  1|T194
  2|T234
  3|T234SLT|T234
  255|T234Presil|T234

!include Platform/NVIDIA/JetsonAndroid/JetsonAndroid.dsc.inc

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFixedAtBuild]
  gNVIDIATokenSpaceGuid.PcdPlatformFamilyName|L"JetsonAndroid"
