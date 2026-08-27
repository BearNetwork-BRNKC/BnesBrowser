/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/crx_file/crx_verifier.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"

#include "base/containers/span.h"

namespace {

// The Brave publisher key that is accepted in addition to upstream's
// kPublisherKeyHash. This key may be used to verify updates of the browser
// itself. If you change this constant, then you will likely also need to change
// the associated file crx-private-key.der, which is not in Git.
// Until May 2024, components were only signed with 0x93, 0x74, 0xd6... Since
// then, they are also signed with this new key. Now, the value here ensures
// that only binaries signed with the new key are accepted.
constexpr uint8_t kBravePublisherKeyHash[] = {
    0xb8, 0xb9, 0xd3, 0x85, 0xd5, 0x1d, 0x37, 0x9d, 0x92, 0x56, 0xa0,
    0xf0, 0xa7, 0xf5, 0x1b, 0xb0, 0x8e, 0x3e, 0xb5, 0x64, 0xab, 0x85,
    0xbd, 0x19, 0xd6, 0xff, 0x49, 0xa7, 0x35, 0x19, 0x84, 0xf7};

auto GetBravePublisherKeyHash() {
  static auto brave_publisher_key = std::to_array(kBravePublisherKeyHash);
  return base::span(brave_publisher_key);
}

// Used in the patch in crx_verifier.cc.
bool IsBravePublisher(base::span<const uint8_t> key_hash) {
  return GetBravePublisherKeyHash() == key_hash;
}

}  // namespace

namespace crx_file {

void SetBravePublisherKeyHashForTesting(base::span<const uint8_t> test_key) {
  GetBravePublisherKeyHash().copy_from(test_key);
}

}  // namespace crx_file

// BNES_GUARD wallet-crx-allow
// BNES: wrap Verify() so a missing Chrome Web Store / Brave publisher proof
// is accepted only for the BNES wallet CRX. This is the check that produces
// CRX_REQUIRED_PROOF_MISSING on drag-drop and on later auto-update CRX
// downloads. The ID string must stay in sync with kBnesWalletExtensionId
// (this target cannot include brave/browser headers).
#define Verify Verify_Chromium
#include <components/crx_file/crx_verifier.cc>
#undef Verify

namespace crx_file {

namespace {

constexpr char kBnesWalletCrxId[] = "mjkhlgmnolenfmeiobklbfclkmbopinj";

bool IsOkCrxResult(VerifierResult result) {
  return result == VerifierResult::OK_FULL || result == VerifierResult::OK_DELTA;
}

}  // namespace

VerifierResult Verify(
    const base::FilePath& crx_path,
    const VerifierFormat& format,
    const std::vector<std::vector<uint8_t>>& required_key_hashes,
    const std::vector<uint8_t>& required_file_hash,
    std::string* public_key,
    std::string* crx_id,
    std::vector<uint8_t>* compressed_verified_contents) {
  std::string local_id;
  std::string* id_out = crx_id ? crx_id : &local_id;

  const VerifierResult result = Verify_Chromium(
      crx_path, format, required_key_hashes, required_file_hash, public_key,
      id_out, compressed_verified_contents);
  if (result != VerifierResult::ERROR_REQUIRED_PROOF_MISSING) {
    return result;
  }
  if (format != VerifierFormat::CRX3_WITH_PUBLISHER_PROOF &&
      format != VerifierFormat::CRX3_WITH_TEST_PUBLISHER_PROOF) {
    return result;
  }

  const VerifierResult unsigned_result = Verify_Chromium(
      crx_path, VerifierFormat::CRX3, required_key_hashes, required_file_hash,
      public_key, id_out, compressed_verified_contents);
  if (IsOkCrxResult(unsigned_result) && *id_out == kBnesWalletCrxId) {
    return unsigned_result;
  }
  return VerifierResult::ERROR_REQUIRED_PROOF_MISSING;
}

}  // namespace crx_file
