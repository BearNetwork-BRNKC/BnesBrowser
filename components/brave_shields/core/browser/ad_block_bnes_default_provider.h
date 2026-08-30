// Copyright (c) 2026 The Bnes. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_BNES_DEFAULT_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_BNES_DEFAULT_PROVIDER_H_

#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"

namespace brave_shields {

class AdBlockBnesDefaultProvider : public AdBlockFiltersProvider {
 public:
  explicit AdBlockBnesDefaultProvider(
      bool engine_is_default,
      AdBlockFiltersProviderManager* filters_provider_manager);
  ~AdBlockBnesDefaultProvider() override;

  AdBlockBnesDefaultProvider(const AdBlockBnesDefaultProvider&) = delete;
  AdBlockBnesDefaultProvider& operator=(const AdBlockBnesDefaultProvider&) =
      delete;

  void LoadFilterSet(
      base::OnceCallback<void(
          base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) override;

  std::string GetNameForDebugging() override;

  bool IsInitialized() const override;

 private:
  bool initialized_ = false;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_BROWSER_AD_BLOCK_BNES_DEFAULT_PROVIDER_H_
