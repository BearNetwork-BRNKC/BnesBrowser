// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
//
// BNES release signer: offline ML-DSA-65 (FIPS 204) signing tool for update
// manifests. See BNES_BROWSER_ARCHITECTURE_EVOLUTION_PLAN.md section 68.
//
// Usage:
//   bnes_release_signer keygen   <seed_file> <pub_file>
//   bnes_release_signer sign     <seed_file> <manifest_file> <sig_file>
//   bnes_release_signer verify   <pub_file>  <manifest_file> <sig_file>
//   bnes_release_signer pubheader <pub_file> <header_out>
//
// The seed file (32 raw bytes) is the private key material and MUST be kept
// offline. All files are raw bytes (no encoding) to keep the pipeline simple
// and byte-exact.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "openssl/bytestring.h"
#include "openssl/mldsa.h"

namespace {

bool ReadFileBytes(const char* path, std::vector<uint8_t>* out) {
  FILE* f = std::fopen(path, "rb");
  if (!f) {
    std::fprintf(stderr, "cannot open %s\n", path);
    return false;
  }
  std::fseek(f, 0, SEEK_END);
  long size = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (size < 0) {
    std::fclose(f);
    return false;
  }
  out->resize(static_cast<size_t>(size));
  size_t read = size ? std::fread(out->data(), 1, out->size(), f) : 0;
  std::fclose(f);
  return read == out->size();
}

bool WriteFileBytes(const char* path, const uint8_t* data, size_t len) {
  FILE* f = std::fopen(path, "wb");
  if (!f) {
    std::fprintf(stderr, "cannot write %s\n", path);
    return false;
  }
  size_t written = len ? std::fwrite(data, 1, len, f) : 0;
  std::fclose(f);
  return written == len;
}

int CmdKeygen(const char* seed_path, const char* pub_path) {
  std::vector<uint8_t> pub(MLDSA65_PUBLIC_KEY_BYTES);
  std::vector<uint8_t> seed(MLDSA_SEED_BYTES);
  struct MLDSA65_private_key priv;
  if (!MLDSA65_generate_key(pub.data(), seed.data(), &priv)) {
    std::fprintf(stderr, "keygen failed\n");
    return 1;
  }
  if (!WriteFileBytes(seed_path, seed.data(), seed.size()) ||
      !WriteFileBytes(pub_path, pub.data(), pub.size())) {
    return 1;
  }
  std::printf(
      "OK: seed (PRIVATE, keep offline) -> %s\n     public key -> %s\n",
      seed_path, pub_path);
  return 0;
}

int CmdSign(const char* seed_path, const char* msg_path, const char* sig_path) {
  std::vector<uint8_t> seed;
  std::vector<uint8_t> msg;
  if (!ReadFileBytes(seed_path, &seed) || seed.size() != MLDSA_SEED_BYTES) {
    std::fprintf(stderr, "bad seed file (must be %d raw bytes)\n",
                 MLDSA_SEED_BYTES);
    return 1;
  }
  if (!ReadFileBytes(msg_path, &msg)) {
    return 1;
  }
  struct MLDSA65_private_key priv;
  if (!MLDSA65_private_key_from_seed(&priv, seed.data(), seed.size())) {
    std::fprintf(stderr, "private key restore failed\n");
    return 1;
  }
  std::vector<uint8_t> sig(MLDSA65_SIGNATURE_BYTES);
  // Empty context; the client verifies with the same empty context.
  if (!MLDSA65_sign(sig.data(), &priv, msg.data(), msg.size(), nullptr, 0)) {
    std::fprintf(stderr, "sign failed\n");
    return 1;
  }
  if (!WriteFileBytes(sig_path, sig.data(), sig.size())) {
    return 1;
  }
  std::printf("OK: signature -> %s\n", sig_path);
  return 0;
}

int CmdVerify(const char* pub_path, const char* msg_path,
              const char* sig_path) {
  std::vector<uint8_t> pub;
  std::vector<uint8_t> msg;
  std::vector<uint8_t> sig;
  if (!ReadFileBytes(pub_path, &pub) ||
      pub.size() != MLDSA65_PUBLIC_KEY_BYTES || !ReadFileBytes(msg_path, &msg) ||
      !ReadFileBytes(sig_path, &sig) || sig.size() != MLDSA65_SIGNATURE_BYTES) {
    std::fprintf(stderr, "bad input files/sizes\n");
    return 1;
  }
  CBS cbs;
  CBS_init(&cbs, pub.data(), pub.size());
  struct MLDSA65_public_key public_key;
  if (!MLDSA65_parse_public_key(&public_key, &cbs)) {
    std::fprintf(stderr, "public key parse failed\n");
    return 1;
  }
  int ok = MLDSA65_verify(&public_key, sig.data(), sig.size(), msg.data(),
                          msg.size(), nullptr, 0);
  std::printf("%s\n", ok ? "VALID" : "INVALID");
  return ok ? 0 : 1;
}

int CmdPubHeader(const char* pub_path, const char* header_path) {
  std::vector<uint8_t> pub;
  if (!ReadFileBytes(pub_path, &pub) ||
      pub.size() != MLDSA65_PUBLIC_KEY_BYTES) {
    std::fprintf(stderr, "bad public key file (must be %d raw bytes)\n",
                 MLDSA65_PUBLIC_KEY_BYTES);
    return 1;
  }
  FILE* f = std::fopen(header_path, "w");
  if (!f) {
    std::fprintf(stderr, "cannot write %s\n", header_path);
    return 1;
  }
  std::fprintf(
      f,
      "// Copyright (c) 2026 The BNES Authors. All rights reserved.\n"
      "// This Source Code Form is subject to the terms of the Mozilla Public\n"
      "// License, v. 2.0. If a copy of the MPL was not distributed with this\n"
      "// file, You can obtain one at https://mozilla.org/MPL/2.0/.\n"
      "//\n"
      "// GENERATED by bnes_release_signer pubheader. Embedded trust root for\n"
      "// the BNES update pipeline (ML-DSA-65, FIPS 204). Do not edit by hand.\n\n"
      "#ifndef BRAVE_BROWSER_BNES_UPDATE_BNES_UPDATE_PUBLIC_KEY_H_\n"
      "#define BRAVE_BROWSER_BNES_UPDATE_BNES_UPDATE_PUBLIC_KEY_H_\n\n"
      "#include <cstdint>\n\n"
      "namespace bnes_update {\n\n"
      "inline constexpr unsigned char kUpdatePublicKey[%d] = {\n",
      MLDSA65_PUBLIC_KEY_BYTES);
  for (size_t i = 0; i < pub.size(); ++i) {
    std::fprintf(f, "0x%02x,", pub[i]);
    if ((i + 1) % 16 == 0) {
      std::fprintf(f, "\n");
    }
  }
  std::fprintf(f, "\n};\n\n}  // namespace bnes_update\n\n#endif\n");
  std::fclose(f);
  std::printf("OK: header -> %s\n", header_path);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 4 && std::strcmp(argv[1], "keygen") == 0) {
    return CmdKeygen(argv[2], argv[3]);
  }
  if (argc == 5 && std::strcmp(argv[1], "sign") == 0) {
    return CmdSign(argv[2], argv[3], argv[4]);
  }
  if (argc == 5 && std::strcmp(argv[1], "verify") == 0) {
    return CmdVerify(argv[2], argv[3], argv[4]);
  }
  if (argc == 4 && std::strcmp(argv[1], "pubheader") == 0) {
    return CmdPubHeader(argv[2], argv[3]);
  }
  std::fprintf(stderr,
               "usage:\n"
               "  %s keygen <seed_file> <pub_file>\n"
               "  %s sign <seed_file> <manifest_file> <sig_file>\n"
               "  %s verify <pub_file> <manifest_file> <sig_file>\n"
               "  %s pubheader <pub_file> <header_out>\n",
               argv[0], argv[0], argv[0], argv[0]);
  return 2;
}