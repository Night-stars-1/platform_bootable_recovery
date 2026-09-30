// SPDX-License-Identifier: Apache-2.0
// Existing-key-only port of vold KeyStorage.cpp; never invokes its upgrade/creation paths.
// Copyright (C) 2016 The Android Open Source Project
#include "key_storage.h"
#include "files.h"
#include "primitives.h"
namespace recovery_crypto::android17 {
int RetrieveExistingKey(Hal& hal, const std::string& directory, View ce_secret, Bytes* key) {
  Bytes version, discardable, encrypted, blob, upgraded;
  bool missing = false;
  if (!ReadFile(directory + "/version", &version, 16)) return RC_EXISTING_KEY_MISSING;
  if (version != Bytes{ '1' }) return RC_UNSUPPORTED;
  if (!ReadFile(directory + "/encrypted_key", &encrypted, 65536)) return RC_EXISTING_KEY_MISSING;
  if (!ReadFile(directory + "/secdiscardable", &discardable, 16384, &missing) && !missing)
    return RC_IO_ERROR;
  // An absent discardable is legitimate for modern software-wrapped CE keys, never DE/metadata.
  if (ce_secret.empty() && discardable.size() != 16384) return RC_EXISTING_KEY_MISSING;
  if (!discardable.empty() && discardable.size() != 16384) return RC_IO_ERROR;
  Bytes discard_hash;
  if (!discardable.empty())
    discard_hash = PersonalizedHash("Android secdiscardable SHA512", discardable);
  auto app_id = Concat(discard_hash, ce_secret);
  if (!ce_secret.empty()) {
    auto wrapping = PersonalizedHash("Android key wrapping key generation SHA512", app_id);
    wrapping.resize(32);
    return GcmDecrypt(wrapping, encrypted, key) ? RC_OK : RC_IO_ERROR;
  }
  // A pending upgraded blob has ambiguous commit status. Leave both versions untouched.
  if (ReadFile(directory + "/keymaster_key_blob_upgraded", &upgraded, 65536, &missing))
    return RC_KEY_UPGRADE_REQUIRED;
  if (!missing) return RC_IO_ERROR;
  if (!ReadFile(directory + "/keymaster_key_blob", &blob, 65536)) return RC_EXISTING_KEY_MISSING;
  return hal.Decrypt(blob, encrypted, app_id, nullptr, key);
}
}  // namespace recovery_crypto::android17
