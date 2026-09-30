/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <optional>
#include "hal.h"
namespace recovery_crypto::android17 {
struct PasswordData {
  int type = -1;
  unsigned n = 0, r = 0, p = 0;
  Bytes salt, handle;
};
bool ParsePasswordData(View bytes, PasswordData* data);
class SyntheticPassword final {
 public:
  int Load(uint32_t user, uint32_t* credential_type);
  int Unlock(Hal& hal, uint32_t credential_type, View credential, Bytes* fbe_key, uint32_t* retry);

 private:
  uint32_t user_ = 0, type_ = RC_CREDENTIAL_NONE;
  uint64_t protector_ = 0;
  Bytes blob_;
  std::optional<PasswordData> password_;
  std::optional<uint32_t> weaver_slot_;
  std::string State(const char* name, bool global = false) const;
};
}  // namespace recovery_crypto::android17
