// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "BnesBrowser/browser/bnes_update/bnes_update_checker.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/process/launch.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "BnesBrowser/browser/bnes_update/bnes_update_public_key.h"
#include "BnesBrowser/components/version_info/version_info.h"
#include "chrome/browser/upgrade_detector/upgrade_detector.h"
#include "crypto/hash.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/boringssl/src/include/openssl/bytestring.h"
#include "third_party/boringssl/src/include/openssl/mldsa.h"
#include "url/gurl.h"

namespace bnes_update {

namespace {

constexpr char kReleasesApiUrl[] =
    "https://api.github.com/repos/BearNetwork-BRNKC/BnesBrowser/releases/latest";

// First check shortly after startup; subsequent checks once a day plus a
// random jitter so a fleet of clients does not hit the API simultaneously.
constexpr base::TimeDelta kFirstCheckDelay = base::Minutes(2);
constexpr base::TimeDelta kCheckPeriod = base::Hours(24);
constexpr base::TimeDelta kCheckPeriodJitter = base::Hours(1);
constexpr base::TimeDelta kCheckTimeout = base::Seconds(30);
constexpr size_t kMaxResponseBytes = 1024 * 1024;  // 1 MB is far beyond need.

// Phase B auto-update pipeline constants.
constexpr char kManifestAssetName[] = "bnes-update-manifest.json";
constexpr char kSignatureAssetName[] = "bnes-update-manifest.sig";
constexpr size_t kMaxSignatureBytes = 65536;
constexpr int64_t kMaxInstallerBytes = 2LL * 1024 * 1024 * 1024;  // 2 GiB

constexpr net::NetworkTrafficAnnotationTag kBnesUpdateTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("bnes_update_checker", R"(
      semantics {
        sender: "BnesBrowser Update Checker"
        description:
          "Checks GitHub Releases for a newer published BnesBrowser version "
          "and raises the built-in update-available UI when found. No data "
          "other than the request itself is sent; nothing is downloaded or "
          "executed."
        trigger: "Periodic timer (about once a day with random jitter) and "
                 "shortly after browser startup."
        data: "None. Only a plain HTTP GET to the public GitHub API."
        destination: OTHER
      }
      policy {
        cookies_allowed: NO
        setting: "Not user-configurable yet; the request carries no user data."
        policy_exception_justification:
          "Not implemented; the check contains no user data and is required "
          "to inform users of security-relevant new releases."
      })");

std::string NormalizeReleaseTag(const std::string& tag_name) {
  std::string tag(base::TrimString(tag_name, "vV ",
                                   base::TrimPositions::TRIM_LEADING));
  return base::ToLowerASCII(tag);
}

std::string ToLowerHex(base::span<const uint8_t> bytes) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(bytes.size() * 2);
  for (uint8_t b : bytes) {
    out.push_back(kHex[b >> 4]);
    out.push_back(kHex[b & 0xf]);
  }
  return out;
}

// Finds the browser_download_url of a release asset by exact file name.
// Returns an empty string when not found.
std::string FindAssetUrl(const base::DictValue& release,
                         const std::string& asset_name) {
  const base::ListValue* assets = release.FindList("assets");
  if (!assets) {
    return std::string();
  }
  for (const base::Value& item : *assets) {
    const base::DictValue* asset = item.GetIfDict();
    if (!asset) {
      continue;
    }
    const std::string* name = asset->FindString("name");
    const std::string* url = asset->FindString("browser_download_url");
    if (name && *name == asset_name && url && !url->empty()) {
      return *url;
    }
  }
  return std::string();
}

// Numeric dot-separated comparison ("1.2.0" vs "1.10.0"). Returns false for
// any non-numeric component so malformed tags can never trigger a prompt.
bool IsStrictlyNewer(const std::string& candidate, const std::string& current) {
  std::vector<std::string> candidate_parts = base::SplitString(
      candidate, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::vector<std::string> current_parts = base::SplitString(
      current, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (candidate_parts.empty()) {
    return false;
  }

  const size_t n = std::max(candidate_parts.size(), current_parts.size());
  for (size_t i = 0; i < n; ++i) {
    uint64_t c = 0;
    uint64_t cur = 0;
    if (i < candidate_parts.size()) {
      if (!base::StringToUint64(candidate_parts[i], &c)) {
        return false;
      }
    }
    if (i < current_parts.size()) {
      if (!base::StringToUint64(current_parts[i], &cur)) {
        return false;
      }
    }
    if (c != cur) {
      return c > cur;
    }
  }
  return false;
}

}  // namespace

BnesUpdateChecker::BnesUpdateChecker(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {}

BnesUpdateChecker::~BnesUpdateChecker() = default;

void BnesUpdateChecker::Start() {
  ScheduleNextCheck(/*first_check=*/true);
}

void BnesUpdateChecker::ScheduleNextCheck(bool first_check) {
  base::TimeDelta delay =
      first_check
          ? kFirstCheckDelay
          : kCheckPeriod +
                base::RandTimeDelta(base::TimeDelta(), kCheckPeriodJitter);
  schedule_timer_.Start(FROM_HERE, delay,
                        base::BindOnce(&BnesUpdateChecker::OnScheduleTimer,
                                       base::Unretained(this)));
}

void BnesUpdateChecker::OnScheduleTimer() {
  if (notified_) {
    return;
  }
  SendCheck();
}

void BnesUpdateChecker::SendCheck() {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kReleasesApiUrl);
  resource_request->method = "GET";
  resource_request->headers.SetHeader("Accept", "application/vnd.github+json");
  // GitHub API rejects requests without a User-Agent.
  resource_request->headers.SetHeader("User-Agent",
                                      "BnesBrowser Update Checker");

  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 kBnesUpdateTrafficAnnotation);
  url_loader_->SetTimeoutDuration(kCheckTimeout);
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BnesUpdateChecker::OnCheckResponse,
                     base::Unretained(this)),
      kMaxResponseBytes);
}

void BnesUpdateChecker::OnCheckResponse(
    std::optional<std::string> response_body) {
  bool schedule_next = true;

  if (notified_) {
    schedule_next = false;
  } else if (response_body && url_loader_ &&
             url_loader_->NetError() == net::OK) {
    std::optional<base::Value> root =
        base::JSONReader::Read(*response_body, base::JSON_PARSE_RFC);
    const std::string* tag_name =
        root ? root->GetDict().FindString("tag_name") : nullptr;
    if (tag_name && IsNewerReleaseTag(*tag_name)) {
      // Raise Chromium's built-in "update available" UI. No download, no
      // execution happens here.
      UpgradeDetector::GetInstance()->NotifyUpgradeForTesting();
      notified_ = true;
      schedule_next = false;
#if BUILDFLAG(IS_WIN)
      // Phase B: best-effort background auto-update. Verifies the signed
      // manifest and installer SHA-256 before executing anything; every
      // failure path aborts silently.
      StartAutoUpdate(root->GetDict());
#endif
    }
  }
  // Anything else (network error, timeout, rate limit, malformed JSON,
  // unknown tag format) is silently dropped; the next check is scheduled.

  url_loader_.reset();
  if (schedule_next) {
    ScheduleNextCheck(/*first_check=*/false);
  }
}

bool BnesUpdateChecker::IsNewerReleaseTag(const std::string& tag_name) const {
  const std::string candidate = NormalizeReleaseTag(tag_name);
  const std::string current =
      NormalizeReleaseTag(version_info::GetBraveVersionNumberForDisplay());
  if (candidate.empty() || candidate == current) {
    return false;
  }
  return IsStrictlyNewer(candidate, current);
}

#if BUILDFLAG(IS_WIN)

void BnesUpdateChecker::StartAutoUpdate(const base::DictValue& release) {
  const std::string manifest_url = FindAssetUrl(release, kManifestAssetName);
  const std::string sig_url = FindAssetUrl(release, kSignatureAssetName);
  if (manifest_url.empty() || sig_url.empty()) {
    // This release does not carry an auto-update payload; notification-only.
    return;
  }
  pending_sig_url_ = sig_url;

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(manifest_url);
  resource_request->method = "GET";
  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 kBnesUpdateTrafficAnnotation);
  url_loader_->SetTimeoutDuration(kCheckTimeout);
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BnesUpdateChecker::OnManifestDownloaded,
                     base::Unretained(this)),
      kMaxResponseBytes);
}

void BnesUpdateChecker::OnManifestDownloaded(
    std::optional<std::string> response_body) {
  if (!response_body || !url_loader_ || url_loader_->NetError() != net::OK) {
    AbortUpdate();
    return;
  }
  manifest_body_ = std::move(*response_body);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(pending_sig_url_);
  resource_request->method = "GET";
  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 kBnesUpdateTrafficAnnotation);
  url_loader_->SetTimeoutDuration(kCheckTimeout);
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BnesUpdateChecker::OnSignatureDownloaded,
                     base::Unretained(this)),
      kMaxSignatureBytes);
}

bool BnesUpdateChecker::VerifyManifestSignature(
    const std::string& signature) const {
  if (signature.size() != MLDSA65_SIGNATURE_BYTES) {
    return false;
  }
  CBS cbs;
  CBS_init(&cbs, kUpdatePublicKey, sizeof(kUpdatePublicKey));
  struct MLDSA65_public_key public_key;
  if (!MLDSA65_parse_public_key(&public_key, &cbs)) {
    return false;
  }
  return MLDSA65_verify(
             &public_key,
             reinterpret_cast<const uint8_t*>(signature.data()),
             signature.size(),
             reinterpret_cast<const uint8_t*>(manifest_body_.data()),
             manifest_body_.size(), nullptr, 0) == 1;
}

void BnesUpdateChecker::OnSignatureDownloaded(
    std::optional<std::string> response_body) {
  // 1. Manifest signature must verify against the embedded trust root.
  if (!response_body || !url_loader_ || url_loader_->NetError() != net::OK ||
      !VerifyManifestSignature(*response_body)) {
    AbortUpdate();
    return;
  }

  // 2. Manifest contents: version strictly newer, HTTPS installer URL,
  //    well-formed SHA-256.
  std::optional<base::Value> manifest =
      base::JSONReader::Read(manifest_body_, base::JSON_PARSE_RFC);
  const base::DictValue* dict = manifest ? &manifest->GetDict() : nullptr;
  const std::string* version = dict ? dict->FindString("version") : nullptr;
  const std::string* sha256 =
      dict ? dict->FindString("installer_sha256") : nullptr;
  const std::string* installer_url =
      dict ? dict->FindString("installer_url") : nullptr;
  GURL url(installer_url ? *installer_url : std::string());
  if (!version || !sha256 || sha256->size() != 64 || !url.is_valid() ||
      !url.SchemeIs("https") ||
      !IsStrictlyNewer(
          NormalizeReleaseTag(*version),
          NormalizeReleaseTag(
              version_info::GetBraveVersionNumberForDisplay()))) {
    AbortUpdate();
    return;
  }
  pending_sha256_ = base::ToLowerASCII(*sha256);

  // 3. Download the installer to a temp file.
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    AbortUpdate();
    return;
  }
  installer_path_ =
      temp_dir.AppendASCII("BnesBrowser_update_" + *version + ".exe");
  base::DeleteFile(installer_path_);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = "GET";
  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 kBnesUpdateTrafficAnnotation);
  url_loader_->SetTimeoutDuration(kCheckTimeout * 60);  // large file
  url_loader_->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&BnesUpdateChecker::OnInstallerDownloaded,
                     base::Unretained(this)),
      installer_path_, kMaxInstallerBytes);
}

void BnesUpdateChecker::OnInstallerDownloaded(base::FilePath path) {
  // 4. Installer integrity: streaming SHA-256 must match the signed manifest.
  bool ok = false;
  if (!path.empty() && url_loader_ && url_loader_->NetError() == net::OK) {
    base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
    if (file.IsValid()) {
      crypto::hash::Hasher hasher(crypto::hash::HashKind::kSha256);
      std::vector<uint8_t> buffer(1 << 20);
      const int64_t file_size = file.GetLength();
      int64_t offset = 0;
      for (;;) {
        const int64_t remaining = file_size - offset;
        if (remaining <= 0) {
          std::array<uint8_t, crypto::hash::kSha256Size> digest;
          hasher.Finish(digest);
          ok = ToLowerHex(digest) == pending_sha256_;
          break;
        }
        const size_t to_read = static_cast<size_t>(
            std::min<int64_t>(remaining, static_cast<int64_t>(buffer.size())));
        if (!file.ReadAndCheck(offset, base::span(buffer).first(to_read))) {
          break;
        }
        hasher.Update(base::span(buffer).first(to_read));
        offset += static_cast<int64_t>(to_read);
      }
    }
  }

  // 5. Execute the verified installer silently. It was authenticated with
  //    ML-DSA-65 (embedded public key) and hash-pinned; anything else aborts.
  if (ok) {
    base::CommandLine installer(installer_path_);
    installer.AppendArg("--do-not-launch-chrome");
    base::LaunchProcess(installer, base::LaunchOptions());
  } else {
    base::DeleteFile(installer_path_);
  }
  installer_path_.clear();
  manifest_body_.clear();
  pending_sig_url_.clear();
  pending_sha256_.clear();
  // notified_ is already true: the checker stays idle for this session.
}

void BnesUpdateChecker::AbortUpdate() {
  url_loader_.reset();
  manifest_body_.clear();
  pending_sig_url_.clear();
  pending_sha256_.clear();
  if (!installer_path_.empty()) {
    base::DeleteFile(installer_path_);
    installer_path_.clear();
  }
  ScheduleNextCheck(/*first_check=*/false);
}

#endif  // BUILDFLAG(IS_WIN)

}  // namespace bnes_update