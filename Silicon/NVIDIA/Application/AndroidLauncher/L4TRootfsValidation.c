/** @file

  Rootfs Validation Library

  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PlatformResourceLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <NVIDIAConfiguration.h>
#include "L4TRootfsValidation.h"

L4T_RF_AB_PARAM  mRootfsInfo = { 0 };

RF_AB_VARIABLE  mRFAbVariable[RF_VARIABLE_INDEX_MAX] = {
  [RF_STATUS_A] =    { L"RootfsStatusSlotA",
                       EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS |
                       EFI_VARIABLE_NON_VOLATILE,
                       sizeof (UINT32),
                       &gNVIDIAPublicVariableGuid },
  [RF_STATUS_B] =    { L"RootfsStatusSlotB",
                       EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS |
                       EFI_VARIABLE_NON_VOLATILE,
                       sizeof (UINT32),
                       &gNVIDIAPublicVariableGuid },
  [RF_RETRY_A] =     { L"RootfsRetrySlotA",
                       EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS |
                       EFI_VARIABLE_NON_VOLATILE,
                       sizeof (UINT32),
                       &gNVIDIAPublicVariableGuid },
  [RF_RETRY_B] =     { L"RootfsRetrySlotB",
                       EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS |
                       EFI_VARIABLE_NON_VOLATILE,
                       sizeof (UINT32),
                       &gNVIDIAPublicVariableGuid },
  [RF_FW_NEXT] =     { L"BootChainFwNext",
                       EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS |
                       EFI_VARIABLE_NON_VOLATILE,
                       sizeof (UINT32),
                       &gNVIDIAPublicVariableGuid },
  [RF_BC_STATUS] =   { L"BootChainFwStatus",
                       EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS |
                       EFI_VARIABLE_NON_VOLATILE,
                       sizeof (UINT32),
                       &gNVIDIAPublicVariableGuid },
};

/**
  Get rootfs A/B related variable according to input index

  @param[in]  VariableIndex       Rootfs A/B related variable index
  @param[out] Value               Value of the variable

  @retval EFI_SUCCESS            The operation completed successfully
  @retval EFI_INVALID_PARAMETER  Input parameter invalid

**/
STATIC
EFI_STATUS
EFIAPI
RFGetVariable (
  IN  RF_VARIABLE_INDEX  VariableIndex,
  OUT UINT32             *Value
  )
{
  RF_AB_VARIABLE  *Variable;
  UINTN           Size;
  EFI_STATUS      Status;

  if (VariableIndex >= RF_VARIABLE_INDEX_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  Variable = &mRFAbVariable[VariableIndex];

  *Value = 0;
  Size   = Variable->Bytes;
  Status = gRT->GetVariable (
                  Variable->Name,
                  Variable->Guid,
                  NULL,
                  &Size,
                  Value
                  );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND)
    {
      // Retry Vars do not exist by default and should default to max
      if ((VariableIndex == RF_RETRY_A) || (VariableIndex == RF_RETRY_B))
      {
        *Value = ROOTFS_RETRY_MAX;
        Status = EFI_SUCCESS;
      // The BootChainFwNext and BootChainFwStatus does not exist by default
      } else if ((VariableIndex == RF_FW_NEXT) || (VariableIndex == RF_BC_STATUS)) {
        DEBUG ((
          DEBUG_INFO,
           "%a: Info: %s is not found\n",
          __FUNCTION__,
          Variable->Name
          ));
        Status = EFI_SUCCESS;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Error getting %s: %r\n",
          __FUNCTION__,
          Variable->Name,
          Status
          ));
      }
    }
  }

  return Status;
}

/**
  Set rootfs A/B related variable according to input index

  @param[in] VariableIndex       Rootfs A/B related variable index
  @param[in] Value               Value of the variable

  @retval EFI_SUCCESS            The operation completed successfully
  @retval EFI_INVALID_PARAMETER  Input parameter invalid

**/
STATIC
EFI_STATUS
EFIAPI
RFSetVariable (
  IN  RF_VARIABLE_INDEX  VariableIndex,
  IN  UINT32             Value
  )
{
  RF_AB_VARIABLE  *Variable;
  EFI_STATUS      Status;

  if (VariableIndex >= RF_VARIABLE_INDEX_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  Variable = &mRFAbVariable[VariableIndex];
  Status   = gRT->SetVariable (
                    Variable->Name,
                    Variable->Guid,
                    Variable->Attributes,
                    Variable->Bytes,
                    &Value
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error setting %s to %u: %r\n",
      __FUNCTION__,
      Variable->Name,
      Value,
      Status
      ));
  }

  return Status;
}

/**
  Delete rootfs A/B related variable according to input index

  @param[in]  VariableIndex       Rootfs A/B related variable index

  @retval EFI_SUCCESS            The operation completed successfully
  @retval EFI_INVALID_PARAMETER  Input parameter invalid

**/
STATIC
EFI_STATUS
EFIAPI
RFDeleteVariable (
  IN  RF_VARIABLE_INDEX  VariableIndex
  )
{
  RF_AB_VARIABLE  *Variable;
  EFI_STATUS      Status;

  if (VariableIndex >= RF_VARIABLE_INDEX_MAX) {
    return EFI_INVALID_PARAMETER;
  }

  Variable = &mRFAbVariable[VariableIndex];
  DEBUG ((DEBUG_INFO, "%a: Deleting %s\n", __FUNCTION__, Variable->Name));

  Status = gRT->SetVariable (
                  Variable->Name,
                  Variable->Guid,
                  Variable->Attributes,
                  0,
                  NULL
                  );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error deleting %s: %r\n",
      __FUNCTION__,
      Variable->Name,
      Status
      ));
  }

  return Status;
}

/**
  Set rootfs status value to mRootfsInfo and set the update flag

  @param[in]  RootfsSlot      Current rootfs slot
  @param[in]  RootfsStatus    Value of rootfs status

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Input parameter invalid

**/
STATIC
EFI_STATUS
EFIAPI
SetStatusTomRootfsInfo (
  IN  UINT32  RootfsSlot,
  IN  UINT32  RootfsStatus
  )
{
  if ((RootfsSlot > ROOTFS_SLOT_B)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RootfsSlot == ROOTFS_SLOT_A) {
    mRootfsInfo.RootfsVar[RF_STATUS_A].Value      = RootfsStatus;
    mRootfsInfo.RootfsVar[RF_STATUS_A].UpdateFlag = 1;
  } else {
    mRootfsInfo.RootfsVar[RF_STATUS_B].Value      = RootfsStatus;
    mRootfsInfo.RootfsVar[RF_STATUS_B].UpdateFlag = 1;
  }

  return EFI_SUCCESS;
}

/**
  Get rootfs retry count from mRootfsInfo

  @param[in]  RootfsSlot          Current rootfs slot
  @param[out] RootfsRetryCount    Rertry count of current RootfsSlot

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Input parameter invalid

**/
STATIC
EFI_STATUS
EFIAPI
GetRetryCountFrommRootfsInfo (
  IN  UINT32  RootfsSlot,
  OUT UINT32  *RootfsRetryCount
  )
{
  if ((RootfsSlot > ROOTFS_SLOT_B) || (RootfsRetryCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RootfsSlot == ROOTFS_SLOT_A) {
    *RootfsRetryCount = mRootfsInfo.RootfsVar[RF_RETRY_A].Value;
  } else {
    *RootfsRetryCount = mRootfsInfo.RootfsVar[RF_RETRY_B].Value;
  }

  return EFI_SUCCESS;
}

/**
  Set rootfs retry count value to mRootfsInfo

  @param[in]  RootfsSlot          Current rootfs slot
  @param[in]  RootfsRetryCount    Rootfs retry count of current RootfsSlot
  @param[out] RootfsInfo          The value of RootfsInfo variable

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Input parameter invalid

**/
STATIC
EFI_STATUS
EFIAPI
SetRetryCountTomRootfsInfo (
  IN  UINT32  RootfsSlot,
  IN  UINT32  RootfsRetryCount
  )
{
  if ((RootfsSlot > ROOTFS_SLOT_B)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RootfsSlot == ROOTFS_SLOT_A) {
    mRootfsInfo.RootfsVar[RF_RETRY_A].Value = RootfsRetryCount;
    mRootfsInfo.RootfsVar[RF_RETRY_A].UpdateFlag = 1;
  } else {
    mRootfsInfo.RootfsVar[RF_RETRY_B].Value = RootfsRetryCount;
    mRootfsInfo.RootfsVar[RF_RETRY_B].UpdateFlag = 1;
  }

  return EFI_SUCCESS;
}

/**
  Check if there is valid rootfs or not

  @retval TRUE     There is valid rootfs
  @retval FALSE    There is no valid rootfs

**/
STATIC
BOOLEAN
EFIAPI
IsValidRootfs (
  VOID
  )
{
  BOOLEAN  Status = TRUE;

  if ((mRootfsInfo.RootfsVar[RF_STATUS_A].Value == ANDROIDLAUNCHER_STATUS_UNBOOTABLE) &&
      (mRootfsInfo.RootfsVar[RF_STATUS_B].Value == ANDROIDLAUNCHER_STATUS_UNBOOTABLE))
  {
    Status = FALSE;
  }

  return Status;
}

/**
  Check mRootfsInfo.RootfsVar, update the variable if UpdateFlag is set

  @retval EFI_SUCCESS    The operation completed successfully.

**/
STATIC
EFI_STATUS
EFIAPI
CheckAndUpdateVariable (
  VOID
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT32      Index;

  // Check mRootfsInfo.RootfsVar[], update the variable if UpdateFlag is set
  for (Index = 0; Index < RF_VARIABLE_INDEX_MAX; Index++) {
    if (mRootfsInfo.RootfsVar[Index].UpdateFlag) {
      Status = RFSetVariable (Index, mRootfsInfo.RootfsVar[Index].Value);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to write: %a\n",
          __FUNCTION__,
          mRFAbVariable[Index].Name
          ));
        return Status;
      }
    }
  }

  return Status;
}

/**
  Check input rootfs slot is bootable or not:
  If the retry count of rootfs slot is not 0, rootfs slot is bootable.
  If the retry count of rootfs slot is 0, rootfs slot is unbootable.

  @param[in] RootfsSlot      The rootfs slot number

  @retval TRUE     The input rootfs slot is bootable
  @retval FALSE    The input rootfs slot is unbootable

**/
STATIC
BOOLEAN
EFIAPI
IsRootfsSlotBootable (
  IN UINT32  RootfsSlot
  )
{
  EFI_STATUS  Status;
  UINT32      RetryCount;
  BOOLEAN     Bootable = FALSE;

  Status = GetRetryCountFrommRootfsInfo (RootfsSlot, &RetryCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Get Rootfs retry count of slot %d from mRootfsInfo: %r\n",
      __FUNCTION__,
      RootfsSlot,
      Status
      ));
    goto Exit;
  }

  // The rootfs slot is bootable.
  if (RetryCount != 0) {
    Bootable = TRUE;
  } else {
    // The rootfs slot is unbootable
    Bootable = FALSE;
  }

Exit:
  return Bootable;
}

/**
  Decrease the RetryCount of input rootfs slot and save to mRootfsInfo

  @param[in] RootfsSlot      The rootfs slot number

  @retval EFI_SUCCESS            The operation completed successfully
  @retval EFI_INVALID_PARAMETER  RetryCount of input rootfs slot is invalid

**/
STATIC
EFI_STATUS
EFIAPI
DecreaseRootfsRetryCount (
  IN UINT32  RootfsSlot
  )
{
  EFI_STATUS  Status;
  UINT32      RetryCount;

  Status = GetRetryCountFrommRootfsInfo (RootfsSlot, &RetryCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Get Rootfs retry count of slot %d from mRootfsInfo: %r\n",
      __FUNCTION__,
      RootfsSlot,
      Status
      ));
    goto Exit;
  }

  // The rootfs slot is bootable.
  if (RetryCount != 0) {
    RetryCount--;
    Status = SetRetryCountTomRootfsInfo (RootfsSlot, RetryCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to set retry count of slot %d to mRootfsInfo: %r\n",
        __FUNCTION__,
        RootfsSlot,
        Status
        ));
      goto Exit;
    }
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

Exit:
  return Status;
}

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
  )
{
  EFI_STATUS  Status;
  UINT32      NonCurrentSlot;
  UINT32      Index;

  // If boot mode has been set to RECOVERY (via runtime service or UEFI menu),
  // boot to recovery
  if (BootParams->BootMode == NVIDIA_L4T_BOOTMODE_RECOVERY) {
    return EFI_SUCCESS;
  }

  if (BootParams->BootChain > ROOTFS_SLOT_B) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid BootChain: %d\n",
      __FUNCTION__,
      BootParams->BootChain
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Read Rootf A/B related variables and store to mRootnfsInfo.RootfsVar[]
  for (Index = 0; Index < RF_VARIABLE_INDEX_MAX; Index++) {
    Status = RFGetVariable (Index, &mRootfsInfo.RootfsVar[Index].Value);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to read: %a\n",
        __FUNCTION__,
        mRFAbVariable[Index].Name
        ));
      return EFI_LOAD_ERROR;
    }
  }

  // When the BootChainOverride value is 0 or 1, the value is set to BootParams->BootChain
  // in ProcessBootParams(), before calling ValidateRootfsStatus()
  mRootfsInfo.CurrentSlot = BootParams->BootChain;
  NonCurrentSlot = !mRootfsInfo.CurrentSlot;

  // Set BootMode to RECOVERY if there is no more valid rootfs
  if (!IsValidRootfs ()) {
    BootParams->BootMode = NVIDIA_L4T_BOOTMODE_RECOVERY;

    return EFI_SUCCESS;
  }

  // Check redundancy level and validate rootfs status
  // Redundancy for both bootloader and rootfs.
  // If current slot is bootable, decrease slot RetryCount by 1 and go on boot;
  // If current slot is unbootable, check non-current slot
  if (IsRootfsSlotBootable (mRootfsInfo.CurrentSlot)) {
    Status = DecreaseRootfsRetryCount (mRootfsInfo.CurrentSlot);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to decrease the RetryCount of slot %d: %r\n",
        __FUNCTION__,
        mRootfsInfo.CurrentSlot,
        Status
        ));
      goto Exit;
    }
    Status = SetStatusTomRootfsInfo (mRootfsInfo.CurrentSlot, ANDROIDLAUNCHER_STATUS_BOOTING);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to set Rootfs status of slot %d to mRootfsInfo: %r\n",
        __FUNCTION__,
        mRootfsInfo.CurrentSlot,
        Status
        ));
      goto Exit;
    }
  } else {
    // Current slot is unbootable, set current slot status as unbootable.
    Status = SetStatusTomRootfsInfo (mRootfsInfo.CurrentSlot, ANDROIDLAUNCHER_STATUS_UNBOOTABLE);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to set Rootfs status of slot %d to mRootfsInfo: %r\n",
        __FUNCTION__,
        mRootfsInfo.CurrentSlot,
        Status
        ));
      goto Exit;
    }

    // Check non-current slot
    if (IsRootfsSlotBootable (NonCurrentSlot)) {
      // Rootfs slot is always linked with bootloader chain
      mRootfsInfo.RootfsVar[RF_FW_NEXT].Value      = NonCurrentSlot;
      mRootfsInfo.RootfsVar[RF_FW_NEXT].UpdateFlag = 1;
    } else {
      // Non-current slot is unbootable, boot to recovery kernel.
      BootParams->BootMode = NVIDIA_L4T_BOOTMODE_RECOVERY;
      Status               = SetStatusTomRootfsInfo (NonCurrentSlot, ANDROIDLAUNCHER_STATUS_UNBOOTABLE);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Failed to set Rootfs status of slot %d to mRootfsInfo: %r\n",
          __FUNCTION__,
          NonCurrentSlot,
          Status
          ));
        goto Exit;
      }
    }
  }

Exit:
  if (Status == EFI_SUCCESS) {
    // Update BootParams->BootChain
    BootParams->BootChain = mRootfsInfo.CurrentSlot;

    // Update the variable if the mRootfsInfo.RootfsVar[x].UpdateFlag is set
    Status = CheckAndUpdateVariable ();
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to check and update variable: %r\n",
        __FUNCTION__,
        Status
        ));
    }

    // Trigger a reset to switch the BootChain if the UpdateFlag of BootChainFwNext is 1
    if (mRootfsInfo.RootfsVar[RF_FW_NEXT].UpdateFlag) {
      // Clear the BootChainFwStatus variable if it exists
      RFDeleteVariable (RF_BC_STATUS);

      Print (L"Switching the bootchain. Resetting the system in 2 seconds.\r\n");
      MicroSecondDelay (2 * DELAY_SECOND);

      ResetCold ();
    }
  }

  return Status;
}
