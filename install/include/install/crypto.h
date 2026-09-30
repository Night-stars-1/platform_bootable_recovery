/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "install/install.h"

// Optional device-tree backend: no entry or service startup without adaptation.
bool RecoveryCryptoAvailable();
InstallResult ApplyFromEncryptedStorage(Device* device);
