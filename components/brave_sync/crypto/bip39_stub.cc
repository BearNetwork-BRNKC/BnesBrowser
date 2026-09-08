/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of this file was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stddef.h>
#include <stdint.h>

extern "C" int bip39_mnemonic_to_bytes(const void* words,
                                       const char* mnemonic,
                                       unsigned char* bytes_out,
                                       size_t len,
                                       size_t* written) {
  return -1;
}

extern "C" int bip39_mnemonic_from_bytes(const void* words,
                                         const unsigned char* bytes,
                                         size_t len,
                                         char** output) {
  return -1;
}

extern "C" int crypto_secretbox_xsalsa20poly1305_tweet(
    unsigned char* ciphertext,
    const unsigned char* message,
    unsigned long long message_len,
    const unsigned char* nonce,
    const unsigned char* key) {
  return -1;
}

extern "C" int crypto_secretbox_xsalsa20poly1305_tweet_open(
    unsigned char* message,
    const unsigned char* ciphertext,
    unsigned long long ciphertext_len,
    const unsigned char* nonce,
    const unsigned char* key) {
  return -1;
}
