/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/files/file_path.h"
#include "BnesBrowser/browser/brave_local_state_prefs.h"
#include "BnesBrowser/browser/brave_profile_prefs.h"
#include "BnesBrowser/browser/brave_stats/buildflags.h"
#include "BnesBrowser/browser/misc_metrics/uptime_monitor_impl.h"
#include "BnesBrowser/browser/translate/brave_translate_prefs_migration.h"
#include "BnesBrowser/components/ai_chat/core/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_account/prefs.h"
#include "BnesBrowser/components/brave_adaptive_captcha/prefs_util.h"
#include "BnesBrowser/components/brave_ads/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_news/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_rewards/core/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_search/browser/backup_results_metrics.h"
#include "BnesBrowser/components/brave_search_conversion/p3a.h"
#include "BnesBrowser/components/brave_shields/content/browser/ad_block_service.h"
#include "BnesBrowser/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "BnesBrowser/components/brave_sync/brave_sync_prefs.h"
#include "BnesBrowser/components/brave_vpn/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_wallet/common/buildflags/buildflags.h"
#include "BnesBrowser/components/constants/pref_names.h"
#include "BnesBrowser/components/ipfs/ipfs_prefs.h"
#include "BnesBrowser/components/l10n/common/prefs.h"
#include "BnesBrowser/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "BnesBrowser/components/ntp_background_images/buildflags/buildflags.h"
#include "BnesBrowser/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "BnesBrowser/components/omnibox/browser/brave_omnibox_prefs.h"
#include "BnesBrowser/components/p3a/metric_log_store.h"
#include "BnesBrowser/components/p3a/rotation_scheduler.h"
#include "BnesBrowser/components/speedreader/common/buildflags/buildflags.h"
#include "BnesBrowser/components/tor/buildflags/buildflags.h"
#include "chrome/common/pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_STATS_UPDATER)
#include "BnesBrowser/browser/brave_stats/brave_stats_updater.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "BnesBrowser/components/ai_chat/core/browser/model_service.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "BnesBrowser/components/brave_ads/core/public/prefs/obsolete_pref_util.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "BnesBrowser/components/brave_news/browser/brave_news_p3a.h"
#include "BnesBrowser/components/brave_news/common/p3a_pref_names.h"
#include "BnesBrowser/components/brave_news/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_NEWS)

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "BnesBrowser/browser/brave_rewards/rewards_prefs_util.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "BnesBrowser/browser/ui/tabs/brave_tab_prefs.h"
#include "BnesBrowser/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_initializer.h"
#include "BnesBrowser/browser/ui/webui/brave_welcome_page/brave_welcome_page_prefs.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "BnesBrowser/components/brave_vpn/common/brave_vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "BnesBrowser/components/brave_wallet/browser/keyring_service.h"
#include "BnesBrowser/components/brave_wallet/browser/pref_names.h"
#include "BnesBrowser/components/decentralized_dns/core/utils.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "BnesBrowser/components/tor/pref_names.h"
#include "BnesBrowser/components/tor/tor_utils.h"
#endif

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "BnesBrowser/browser/gcm_driver/brave_gcm_utils.h"
#endif

#if BUILDFLAG(ENABLE_CUSTOM_BACKGROUND)
#include "BnesBrowser/browser/ntp_background/ntp_background_prefs.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "BnesBrowser/components/sidebar/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "BnesBrowser/components/speedreader/speedreader_pref_migration.h"
#endif

#if !BUILDFLAG(ENABLE_EXTENSIONS)
// CHROMIUM_SRC_NOLINT
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)

namespace {

// BEGIN_MIGRATE_OBSOLETE_PROFILE_PREFS

// END_MIGRATE_OBSOLETE_PROFILE_PREFS

// BEGIN_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS

// END_MIGRATE_OBSOLETE_LOCAL_STATE_PREFS

}  // namespace

#include <chrome/browser/prefs/browser_prefs.cc>

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#undef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)
