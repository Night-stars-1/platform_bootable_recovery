/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
// Pure format/crypto fixtures only. No test calls a HAL, the backend or real key paths.
#include <gtest/gtest.h>
#include "primitives.h"
#include "sqlite_snapshot.h"
#include "synthetic_password.h"
#include "vectors.h"
using namespace recovery_crypto::android17;
static Bytes Hex(const char* value) {
  Bytes out;
  auto digit = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
  for (size_t i = 0; value[i]; i += 2) out.push_back((digit(value[i]) << 4) | digit(value[i + 1]));
  return out;
}
TEST(Android17RecoveryCrypto, SyntheticPasswordVersionedSubkeys) {
  auto sp = AsBytes(kSyntheticPassword);
  EXPECT_EQ(SpSubkey(1, sp, "fbe-key"), Hex(kFbeV2));
  EXPECT_EQ(SpSubkey(2, sp, "fbe-key"), Hex(kFbeV2));
  EXPECT_EQ(SpSubkey(3, sp, "fbe-key"), Hex(kFbeV3));
  EXPECT_TRUE(SpSubkey(4, sp, "fbe-key").empty());
  EXPECT_NE(SpSubkey(3, sp, "fbe-key"), SpSubkey(3, sp, "sp-gk-authentication"));
}
TEST(Android17RecoveryCrypto, ScryptUsesStoredParameters) {
  Bytes out;
  ASSERT_TRUE(Stretch(AsBytes("123456"), Hex("000102030405060708090a0b0c0d0e0f"), 9, 3, 1, &out));
  EXPECT_EQ(out, Hex(kStretchedPin));
  EXPECT_FALSE(Stretch(AsBytes("123456"), Hex("00"), 255, 3, 1, &out));
  EXPECT_FALSE(Stretch(AsBytes("123456"), Hex("00"), 20, 8, 8, &out));
}
TEST(Android17RecoveryCrypto, AesGcmAuthenticatesBeforePublishingPlaintext) {
  // BoringSSL aes_256_gcm_tests.txt, first vector: empty plaintext, no AAD.
  auto key = Hex("e5ac4a32c67e425ac4b143c83c6f161312a97d88d634afdf9f4da5bd35223f01");
  auto blob = Hex("5bf11a0951f0bfc7ea5c9e58d7cba289d6d19a5af45dc13857016bac");
  Bytes out{ 1, 2, 3 };
  ASSERT_TRUE(GcmDecrypt(key, blob, &out));
  EXPECT_TRUE(out.empty());
  blob.back() ^= 1;
  out = { 1, 2, 3 };
  EXPECT_FALSE(GcmDecrypt(key, blob, &out));
  EXPECT_TRUE(out.empty());
  blob.resize(12);
  EXPECT_FALSE(GcmDecrypt(key, blob, &out));
}
TEST(Android17RecoveryCrypto, PasswordDataTruncationAndBetaFormat) {
  auto valid = Hex(kPasswordData);
  PasswordData data;
  ASSERT_TRUE(ParsePasswordData(valid, &data));
  EXPECT_EQ(data.type, 3);
  EXPECT_EQ(data.salt.size(), 16u);
  EXPECT_EQ(data.handle.size(), 9u);
  for (size_t length = 0; length < valid.size() - 4; ++length)
    EXPECT_FALSE(ParsePasswordData(View(valid).first(length), &data));
  auto beta = valid;
  beta[0] = 0;
  beta[1] = 2;
  EXPECT_TRUE(ParsePasswordData(beta, &data));
  EXPECT_EQ(data.type, 3);
  valid[7] = 0xff;
  EXPECT_FALSE(ParsePasswordData(valid, &data));
}
TEST(Android17RecoveryCrypto, WalCommitsAndChecksums) {
  auto database = Hex(kDatabase), wal = Hex(kWal), expected = Hex(kCommittedDatabase);
  ASSERT_TRUE(ApplyCommittedWal(&database, wal));
  EXPECT_EQ(database, expected);
  auto damaged = wal;
  damaged[damaged.size() - 1] ^= 1;
  database = Hex(kDatabase);
  EXPECT_FALSE(ApplyCommittedWal(&database, damaged));
  database = Hex(kDatabase);
  EXPECT_TRUE(ApplyCommittedWal(&database, View(wal).first(wal.size() - 1)));
  EXPECT_EQ(database, Hex(kDatabase));  // Incomplete final commit frame must not be applied.
}
