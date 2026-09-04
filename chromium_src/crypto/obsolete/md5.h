/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CRYPTO_OBSOLETE_MD5_H_
#define BRAVE_CHROMIUM_SRC_CRYPTO_OBSOLETE_MD5_H_

#include <array>
#include <cstdint>

#include "base/containers/span.h"

namespace brave {
std::array<uint8_t, 16> Md5ForDefaultProtocolHandler(
    base::span<const uint8_t> data);
}

// Chromium 152 lists Md5 friends with FRIEND_TEST_ALL_PREFIXES(Md5Test,
// KnownAnswer). Hijacking KnownAnswer expands inside that gtest macro and
// produces "undeclared identifier 'brave'". Inject the friend next to the
// unique HashForTesting() declaration instead.
#define HashForTesting(arg)                                              \
  HashForTesting(arg);                                                   \
  friend std::array<uint8_t, kSize> brave::Md5ForDefaultProtocolHandler( \
      base::span<const uint8_t> data)

#define BRAVE_CRYPTO_OBSOLETE_MD5

#include <crypto/obsolete/md5.h>  // IWYU pragma: export

#undef BRAVE_CRYPTO_OBSOLETE_MD5
#undef HashForTesting

#endif  // BRAVE_CHROMIUM_SRC_CRYPTO_OBSOLETE_MD5_H_
