// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BNES_UPDATE_BNES_UPDATE_CHECKER_H_
#define BRAVE_BROWSER_BNES_UPDATE_BNES_UPDATE_CHECKER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace bnes_update {

// Phase A updater (BNES_BROWSER_ARCHITECTURE_EVOLUTION_PLAN.md section 68.3):
// periodically polls the GitHub Releases API and, when a newer product release
// tag is published, raises Chromium's built-in "update available" UI through
// UpgradeDetector. This service never downloads or executes an installer; all
// network and parsing failures are silent and simply postpone the next check.
class BnesUpdateChecker {
 public:
  explicit BnesUpdateChecker(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  BnesUpdateChecker(const BnesUpdateChecker&) = delete;
  BnesUpdateChecker& operator=(const BnesUpdateChecker&) = delete;

  ~BnesUpdateChecker();

  // Schedules the first check. Must be called once, on the UI thread, after
  // the network stack is up (PostBrowserStart).
  void Start();

 private:
  void ScheduleNextCheck(bool first_check);
  void OnScheduleTimer();
  void SendCheck();
  void OnCheckResponse(std::optional<std::string> response_body);

  // Phase B (background auto-update, plan section 68.3): download the signed
  // release manifest, verify it against the embedded ML-DSA-65 public key,
  // download the installer, verify its SHA-256, and run it silently. Any
  // failure at any step aborts the update silently and nothing is executed.
  void StartAutoUpdate(const base::DictValue& release);
  void OnManifestDownloaded(std::optional<std::string> response_body);
  void OnSignatureDownloaded(std::optional<std::string> response_body);
  void OnInstallerDownloaded(base::FilePath path);
  // Returns true if manifest_body_ carries a valid ML-DSA-65 signature over
  // its exact bytes (empty signing context), made by the embedded key.
  bool VerifyManifestSignature(const std::string& signature) const;
  // Aborts the current update attempt: deletes any partial download and
  // schedules the next check.
  void AbortUpdate();

  // Returns true if |tag_name| is a valid product release tag strictly newer
  // than the running product version. Returns false for anything unrecognized
  // (never notifies on ambiguity).
  bool IsNewerReleaseTag(const std::string& tag_name) const;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  base::OneShotTimer schedule_timer_;

  // Set after the first successful notification; the checker stops rescheduling
  // afterwards so the user is prompted at most once per session.
  bool notified_ = false;

  // Phase B state.
  std::string release_tag_;
  std::string pending_sig_url_;
  std::string pending_sha256_;
  std::string manifest_body_;
  base::FilePath installer_path_;
};

}  // namespace bnes_update

#endif  // BRAVE_BROWSER_BNES_UPDATE_BNES_UPDATE_CHECKER_H_