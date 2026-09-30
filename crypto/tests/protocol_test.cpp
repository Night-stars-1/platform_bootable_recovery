// SPDX-License-Identifier: Apache-2.0
#include "protocol.h"
#include <gtest/gtest.h>
#include <cstring>
#include <limits>

namespace recovery_crypto {
TEST(RecoveryCryptoProtocol, RejectsUnknownMessagesAndOversizedCredentials) {
  Request r;
  r.operation = kPrepare;
  ASSERT_TRUE(ValidRequest(r));
  r.protocol = 99;
  EXPECT_FALSE(ValidRequest(r));
  r.protocol = kProtocol;
  r.operation = 99;
  EXPECT_FALSE(ValidRequest(r));
  r.operation = kUnlock;
  r.length = RECOVERY_CRYPTO_MAX_CREDENTIAL + 1;
  EXPECT_FALSE(ValidRequest(r));
  r.length = std::numeric_limits<uint32_t>::max();
  EXPECT_FALSE(ValidRequest(r));
  r.length = 0;
  r.credential_type = 999;
  EXPECT_FALSE(ValidRequest(r));
}
TEST(RecoveryCryptoProtocol, PreparationNeverCarriesCredentials) {
  Request r;
  r.operation = kPrepare;
  r.length = 1;
  EXPECT_FALSE(ValidRequest(r));
  r.length = 0;
  r.credential_type = RC_CREDENTIAL_PIN;
  EXPECT_FALSE(ValidRequest(r));
}
TEST(RecoveryCryptoProtocol, SeparatesPinAndAndroidPatternEncoding) {
  Request pin;
  pin.operation = kUnlock;
  pin.credential_type = RC_CREDENTIAL_PIN;
  pin.length = 4;
  memcpy(pin.credential, "1234", 4);
  EXPECT_TRUE(ValidRequest(pin));
  pin.credential[1] = 'a';
  EXPECT_FALSE(ValidRequest(pin));
  Request pattern;
  pattern.operation = kUnlock;
  pattern.credential_type = RC_CREDENTIAL_PATTERN;
  pattern.length = 4;
  for (uint8_t i = 0; i < 4; ++i) pattern.credential[i] = i;
  EXPECT_TRUE(ValidRequest(pattern));
  pattern.credential[3] = pattern.credential[1];
  EXPECT_FALSE(ValidRequest(pattern));
  pattern.credential[3] = '4';
  EXPECT_FALSE(ValidRequest(pattern));
  pattern.length = 3;
  EXPECT_FALSE(ValidRequest(pattern));
}
TEST(RecoveryCryptoProtocol, RejectsFalseThrottleAndUnrecognizedReplies) {
  Reply r;
  ASSERT_TRUE(ValidReply(r));
  r.status = static_cast<uint32_t>(Status::Throttled);
  EXPECT_FALSE(ValidReply(r));
  r.retry_seconds = 30;
  EXPECT_TRUE(ValidReply(r));
  r.kind = 99;
  EXPECT_FALSE(ValidReply(r));
  r.kind = kResult;
  r.stage = 99;
  EXPECT_FALSE(ValidReply(r));
  EXPECT_EQ(Status::InvalidBackend, BackendStatus(-1));
  EXPECT_EQ(Status::MissingKey, BackendStatus(RC_EXISTING_KEY_MISSING));
  EXPECT_EQ(Status::UpgradeRequired, BackendStatus(RC_KEY_UPGRADE_REQUIRED));
}
}  // namespace recovery_crypto
