// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"

#include <string>
#include <utility>

#include "base/trace_event/trace_event.h"

namespace {

constexpr char kBnesDefaultCatalogJson[] = R"([
  {
    "uuid": "bnes-default-filter-list",
    "url": "",
    "title": "BNES Default Filter List",
    "langs": ["en"],
    "support_url": "",
    "desc": "Bundled default filter list for BNES (EasyList, EasyPrivacy, uBO, Brave, Cookie).",
    "hidden": false,
    "default_enabled": true,
    "first_party_protections": false,
    "permission_mask": 255,
    "platforms": ["win32", "linux", "mac"],
    "component_id": "",
    "base64_public_key": ""
  }
])";

}  // namespace

namespace brave_shields {

AdBlockFilterListCatalogProvider::AdBlockFilterListCatalogProvider(
    component_updater::ComponentUpdateService* cus) {}

AdBlockFilterListCatalogProvider::~AdBlockFilterListCatalogProvider() = default;

void AdBlockFilterListCatalogProvider::AddObserver(
    AdBlockFilterListCatalogProvider::Observer* observer) {
  observers_.AddObserver(observer);
}

void AdBlockFilterListCatalogProvider::RemoveObserver(
    AdBlockFilterListCatalogProvider::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded(
    const std::string& catalog_json) {
  TRACE_EVENT("brave.adblock",
              "AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded",
              perfetto::TerminatingFlow::FromPointer(this), "catalog_json_size",
              catalog_json.size());
  for (auto& observer : observers_) {
    observer.OnFilterListCatalogLoaded(catalog_json);
  }
}

void AdBlockFilterListCatalogProvider::LoadFilterListCatalog(
    base::OnceCallback<void(const std::string& catalog_json)> cb) {
  std::move(cb).Run(kBnesDefaultCatalogJson);
}

}  // namespace brave_shields
