// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "install/install.h"

// Optional device-tree backend: no entry or service startup without adaptation.
bool RecoveryCryptoAvailable();
// Unlock user 0 using the existing credential UI. Missing adaptation or user
// cancellation returns false; callers keep ordinary Recovery actions available.
// Recheck real storage access so later ZIP selection can reuse installed keys.
bool UnlockRecoveryStorage(Device* device);
InstallResult ApplyFromEncryptedStorage(Device* device);
