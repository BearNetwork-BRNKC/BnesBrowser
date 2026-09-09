// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "BnesBrowser/browser/bnes_update/bnes_update_checker.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "BnesBrowser/components/version_info/version_info.h"
#include "chrome/browser/upgrade_detector/upgrade_detector.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
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
      // execution: the user is pointed at the GitHub Releases page.
      UpgradeDetector::GetInstance()->NotifyUpgradeForTesting();
      notified_ = true;
      schedule_next = false;
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

}  // namespace bnes_update