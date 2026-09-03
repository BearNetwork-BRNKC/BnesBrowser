/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BnesBrowser/browser/android/safe_browsing/features.h"
#include "BnesBrowser/browser/android/youtube_script_injector/features.h"
#include "BnesBrowser/browser/brave_browser_features.h"
#include "BnesBrowser/components/ai_chat/core/common/features.h"
#include "BnesBrowser/components/brave_ads/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_news/common/features.h"
#include "BnesBrowser/components/brave_origin/features.h"
#include "BnesBrowser/components/brave_rewards/core/features.h"
#include "BnesBrowser/components/brave_search_conversion/features.h"
#include "BnesBrowser/components/brave_shields/core/common/features.h"
#include "BnesBrowser/components/brave_vpn/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_wallet/common/buildflags/buildflags.h"
#include "BnesBrowser/components/debounce/core/common/features.h"
#include "BnesBrowser/components/email_aliases/buildflags/buildflags.h"
#include "BnesBrowser/components/google_sign_in_permission/features.h"
#include "BnesBrowser/components/ntp_background_images/browser/features.h"
#include "BnesBrowser/components/playlist/core/common/features.h"
#include "BnesBrowser/components/request_otr/common/features.h"
#include "BnesBrowser/components/web_discovery/buildflags/buildflags.h"
#include "BnesBrowser/components/webcompat/core/common/features.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "BnesBrowser/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "BnesBrowser/components/brave_vpn/common/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "BnesBrowser/components/brave_wallet/common/features.h"
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "BnesBrowser/components/email_aliases/features.h"
#endif

// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_AI_CHAT_FLAGS                                        \
  &ai_chat::features::kAIChat, &ai_chat::features::kAIChatHistory, \
      &ai_chat::features::kBraveSyncAIChat,

#if BUILDFLAG(ENABLE_BRAVE_ADS)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_NEW_TAB_PAGE_AD_FLAG &brave_ads::kNewTabPageAdFeature,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_NEW_TAB_PAGE_AD_FLAG
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "BnesBrowser/components/web_discovery/common/features.h"
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_WEB_DISCOVERY_FLAG \
  &web_discovery::features::kBraveWebDiscoveryNative,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_WEB_DISCOVERY_FLAG
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_VPN_FLAG &brave_vpn::features::kBraveVPNLinkSubscriptionAndroidUI,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_VPN_FLAG
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
// CHROMIUM_SRC_INTERNAL_USE
#define EMAIL_ALIASES_FLAG &email_aliases::features::kEmailAliases,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define EMAIL_ALIASES_FLAG
#endif

// clang-format off
#define kForceWebContentsDarkMode kForceWebContentsDarkMode,                   \
    BRAVE_AI_CHAT_FLAGS                                                        \
    BRAVE_NEW_TAB_PAGE_AD_FLAG                                                 \
    BRAVE_WEB_DISCOVERY_FLAG                                                   \
    BRAVE_VPN_FLAG                                                             \
    EMAIL_ALIASES_FLAG                                                         \
    &brave_rewards::features::kBraveRewards,                                   \
    &brave_search_conversion::features::kOmniboxBanner,                        \
    &playlist::features::kPlaylist,                                            \
    &download::features::kParallelDownloading,                                 \
    &preferences::features::kBraveBackgroundVideoPlayback,                     \
    &preferences::features::kBravePictureInPictureForYouTubeVideos,            \
    &request_otr::features::kBraveRequestOTRTab,                               \
    &safe_browsing::features::kBraveAndroidSafeBrowsing,                       \
    &debounce::features::kBraveDebounce,                                       \
    &webcompat::features::kBraveWebcompatExceptionsService,                    \
    &net::features::kBraveHttpsByDefault,                                      \
    &net::features::kBraveFallbackDoHProvider,                                 \
    &google_sign_in_permission::features::kBraveGoogleSignInPermission,        \
    &net::features::kBraveForgetFirstPartyStorage,                             \
    &brave_shields::features::kBraveShowStrictFingerprintingMode,              \
    &brave_shields::features::kBlockAllCookiesToggle,                          \
    &brave_shields::features::kBraveShieldsElementPicker,                      \
    &features::kBraveAndroidDynamicColorsByDefault,                            \
    &features::kBraveFreshNtpAfterIdleExperiment,                              \
    &ntp_background_images::features::kBraveNTPBrandedWallpaperSurveyPanelist, \
    &brave_shields::features::kBraveShredFeature,                              \
    &brave_origin::features::kBraveOrigin,                                     \
    &features::kBraveCustomSearchEngines,                                      \
    &features::kBraveAndroidTabGroupsSettings

// clang-format on

#include <chrome/browser/flags/android/chrome_feature_list.cc>
#undef kForceWebContentsDarkMode
#undef BRAVE_AI_CHAT_FLAGS
#undef BRAVE_NEW_TAB_PAGE_AD_FLAG
#undef BRAVE_WEB_DISCOVERY_FLAG
#undef BRAVE_VPN_FLAG
#undef EMAIL_ALIASES_FLAG
