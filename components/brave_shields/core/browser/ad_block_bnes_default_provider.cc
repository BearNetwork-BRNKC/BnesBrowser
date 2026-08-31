// Copyright (c) 2026 The Bnes. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_bnes_default_provider.h"

#include <utility>

#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/bnes_default_list.h"

namespace {

void AddBnesBundledListToFilterSet(rust::Box<adblock::FilterSet>* filter_set) {
  constexpr uint8_t kAllPermissions = 0xff;
  const unsigned char* begin = std::begin(bnes_filterlist::kBnesDefaultList);
  const unsigned char* end = std::end(bnes_filterlist::kBnesDefaultList);
  brave_component_updater::DATFileDataBuffer buffer(begin, end);
  (*filter_set)->add_filter_list_with_permissions(buffer, kAllPermissions);
}

}  // namespace

namespace brave_shields {

AdBlockBnesDefaultProvider::AdBlockBnesDefaultProvider(
    bool engine_is_default,
    AdBlockFiltersProviderManager* filters_provider_manager)
    : AdBlockFiltersProvider(engine_is_default, filters_provider_manager),
      initialized_(true) {}

AdBlockBnesDefaultProvider::~AdBlockBnesDefaultProvider() = default;

void AdBlockBnesDefaultProvider::LoadFilterSet(
    base::OnceCallback<void(
        base::OnceCallback<void(rust::Box<adblock::FilterSet>*)>)> cb) {
  std::move(cb).Run(base::BindOnce(&AddBnesBundledListToFilterSet));
}

std::string AdBlockBnesDefaultProvider::GetNameForDebugging() {
  return "AdBlockBnesDefaultProvider";
}

bool AdBlockBnesDefaultProvider::IsInitialized() const {
  return initialized_;
}

}  // namespace brave_shields
