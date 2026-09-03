/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BnesBrowser/browser/browser_context_keyed_service_factories.h"

#include "base/feature_list.h"
#include "BnesBrowser/browser/brave_account/brave_account_service_factory.h"
#include "BnesBrowser/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "BnesBrowser/browser/brave_origin/brave_origin_service_factory.h"
#include "BnesBrowser/browser/brave_search/backup_results_service_factory.h"
#include "BnesBrowser/browser/brave_shields/ad_block_pref_service_factory.h"
#include "BnesBrowser/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "BnesBrowser/browser/debounce/debounce_service_factory.h"
#include "BnesBrowser/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "BnesBrowser/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "BnesBrowser/browser/ntp_background/view_counter_service_factory.h"
#include "BnesBrowser/browser/permissions/permission_lifetime_manager_factory.h"
#include "BnesBrowser/browser/profiles/brave_renderer_updater_factory.h"
#include "BnesBrowser/browser/search_engines/search_engine_provider_service_factory.h"
#include "BnesBrowser/browser/search_engines/search_engine_tracker.h"
#include "BnesBrowser/browser/serp_metrics/serp_metrics_service_factory.h"
#include "BnesBrowser/browser/skus/skus_service_factory.h"
#include "BnesBrowser/browser/sync/brave_sync_alerts_service_factory.h"
#include "BnesBrowser/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "BnesBrowser/browser/webcompat_reporter/webcompat_reporter_service_factory.h"
#include "BnesBrowser/components/ai_chat/core/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_account/features.h"
#include "BnesBrowser/components/brave_ads/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_news/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_perf_predictor/browser/named_third_party_registry_factory.h"
#include "BnesBrowser/components/brave_rewards/core/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_vpn/common/buildflags/buildflags.h"
#include "BnesBrowser/components/brave_wallet/common/buildflags/buildflags.h"
#include "BnesBrowser/components/commander/common/buildflags/buildflags.h"
#include "BnesBrowser/components/containers/buildflags/buildflags.h"
#include "BnesBrowser/components/email_aliases/buildflags/buildflags.h"
#include "BnesBrowser/components/playlist/core/common/buildflags/buildflags.h"
#include "BnesBrowser/components/psst/buildflags/buildflags.h"
#include "BnesBrowser/components/request_otr/common/buildflags/buildflags.h"
#include "BnesBrowser/components/speedreader/common/buildflags/buildflags.h"
#include "BnesBrowser/components/tor/buildflags/buildflags.h"
#include "BnesBrowser/components/traffic_control/buildflags/buildflags.h"
#include "BnesBrowser/components/web_discovery/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "BnesBrowser/browser/ai_chat/ai_chat_service_factory.h"
#include "BnesBrowser/browser/ai_chat/model_service_factory.h"
#include "BnesBrowser/browser/ai_chat/ollama/ollama_service_factory.h"
#include "BnesBrowser/browser/ai_chat/tab_tracker_service_factory.h"
#include "BnesBrowser/components/ai_chat/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "BnesBrowser/browser/brave_ads/ads_service_factory.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "BnesBrowser/browser/brave_rewards/rewards_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "BnesBrowser/browser/brave_vpn/brave_vpn_service_factory.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "BnesBrowser/browser/ui/bookmark/bookmark_prefs_service_factory.h"
#include "BnesBrowser/browser/ui/commands/accelerator_service_factory.h"
#include "BnesBrowser/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "BnesBrowser/browser/workspaces/workspace_service_factory.h"
#include "BnesBrowser/components/commands/common/features.h"
#include "chrome/browser/ui/tabs/features.h"
#else
#include "BnesBrowser/browser/brave_shields/filter_list_service_factory.h"
#include "BnesBrowser/browser/ntp_background/android/ntp_background_images_bridge.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "BnesBrowser/browser/tor/tor_profile_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
#include "BnesBrowser/browser/ui/commander/commander_service_factory.h"
#include "BnesBrowser/components/commander/common/features.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "BnesBrowser/browser/ui/sidebar/sidebar_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "BnesBrowser/browser/speedreader/speedreader_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "BnesBrowser/browser/request_otr/request_otr_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "BnesBrowser/browser/web_discovery/web_discovery_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "BnesBrowser/browser/extensions/manifest_v2/brave_extensions_manifest_v2_migrator.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "BnesBrowser/browser/brave_wallet/brave_wallet_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "BnesBrowser/browser/brave_news/brave_news_controller_factory.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "BnesBrowser/browser/psst/psst_reporter_service_factory.h"
#include "BnesBrowser/browser/psst/psst_settings_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "BnesBrowser/browser/containers/containers_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_TRAFFIC_CONTROL)
#include "BnesBrowser/browser/traffic_control/traffic_control_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "BnesBrowser/browser/email_aliases/email_aliases_service_factory.h"
#include "BnesBrowser/components/email_aliases/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "BnesBrowser/browser/playlist/playlist_service_factory.h"
#endif

namespace brave {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetInstance();
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  brave_ads::AdsServiceFactory::GetInstance();
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
  brave_origin::BraveOriginServiceFactory::GetInstance();
  brave_perf_predictor::NamedThirdPartyRegistryFactory::GetInstance();
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  brave_rewards::RewardsServiceFactory::GetInstance();
#endif
  brave_shields::AdBlockPrefServiceFactory::GetInstance();
  debounce::DebounceServiceFactory::GetInstance();
  brave::URLSanitizerServiceFactory::GetInstance();
  BraveRendererUpdaterFactory::GetInstance();
  SearchEngineProviderServiceFactory::GetInstance();
  misc_metrics::ProfileMiscMetricsServiceFactory::GetInstance();
#if BUILDFLAG(ENABLE_TOR)
  TorProfileServiceFactory::GetInstance();
#endif
  SearchEngineTrackerFactory::GetInstance();
  ntp_background_images::ViewCounterServiceFactory::GetInstance();

#if !BUILDFLAG(IS_ANDROID)
  BookmarkPrefsServiceFactory::GetInstance();
#else
  brave_shields::FilterListServiceFactory::GetInstance();
  ntp_background_images::NTPBackgroundImagesBridgeFactory::GetInstance();
#endif

  webcompat_reporter::WebcompatReporterServiceFactory::GetInstance();

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  brave_news::BraveNewsControllerFactory::GetInstance();
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  brave_wallet::BraveWalletServiceFactory::GetInstance();
#endif

#if !BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    commands::AcceleratorServiceFactory::GetInstance();
  }
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
  if (base::FeatureList::IsEnabled(features::kBraveCommander)) {
    commander::CommanderServiceFactory::GetInstance();
  }
#endif

  EphemeralStorageServiceFactory::GetInstance();
  PermissionLifetimeManagerFactory::GetInstance();
  skus::SkusServiceFactory::GetInstance();
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVpnServiceFactory::GetInstance();
#endif
#if BUILDFLAG(ENABLE_PLAYLIST)
  // Always instantiate the factory so Playlist prefs are registered, even when
  // the feature flag is off. The factory itself returns null from
  // BuildServiceInstanceForBrowserContext when the feature is disabled.
  playlist::PlaylistServiceFactory::GetInstance();
#endif
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  request_otr::RequestOTRServiceFactory::GetInstance();
#endif

  BraveSyncAlertsServiceFactory::GetInstance();

#if BUILDFLAG(ENABLE_CONTAINERS)
  ContainersServiceFactory::GetInstance();
#endif

#if BUILDFLAG(ENABLE_TRAFFIC_CONTROL)
  TrafficControlServiceFactory::GetInstance();
#endif

#if !BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
    SharedPinnedTabServiceFactory::GetInstance();
  }
#endif

#if defined(TOOLKIT_VIEWS)
  sidebar::SidebarServiceFactory::GetInstance();
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderServiceFactory::GetInstance();
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::features::IsAIChatEnabled()) {
    ai_chat::AIChatServiceFactory::GetInstance();
    ai_chat::ModelServiceFactory::GetInstance();
    ai_chat::OllamaServiceFactory::GetInstance();
    ai_chat::TabTrackerServiceFactory::GetInstance();
  }
#endif

  brave_search::BackupResultsServiceFactory::GetInstance();

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  web_discovery::WebDiscoveryServiceFactory::GetInstance();
#endif

  if (brave_account::features::IsBraveAccountEnabled()) {
    brave_account::BraveAccountServiceFactory::GetInstance();
  }

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
  if (email_aliases::features::IsEmailAliasesEnabled()) {
    email_aliases::EmailAliasesServiceFactory::GetInstance();
  }
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_mv2::ExtensionsManifestV2MigratorFactory::GetInstance();
#endif
  BraveShieldsSettingsServiceFactory::GetInstance();

#if BUILDFLAG(ENABLE_PSST)
  PsstSettingsServiceFactory::GetInstance();
  PsstReporterServiceFactory::GetInstance();
#endif  // BUILDFLAG(ENABLE_PSST)

  serp_metrics::SerpMetricsServiceFactory::GetInstance();

#if !BUILDFLAG(IS_ANDROID)
  WorkspaceServiceFactory::GetInstance();
#endif
}

}  // namespace brave
