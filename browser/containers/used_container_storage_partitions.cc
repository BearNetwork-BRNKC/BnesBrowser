// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "BnesBrowser/browser/containers/used_container_storage_partitions.h"

#include <string>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "BnesBrowser/browser/containers/containers_service_factory.h"
#include "BnesBrowser/components/containers/content/browser/storage_partition_utils.h"
#include "BnesBrowser/components/containers/core/browser/containers_service.h"
#include "BnesBrowser/components/containers/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/storage_partition_config.h"

namespace containers {

std::vector<content::StoragePartitionConfig>
GetUsedContainerStoragePartitionConfigs(Profile* profile) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));

  if (!profile) {
    return {};
  }

  ContainersService* service = ContainersServiceFactory::GetForProfile(profile);
  if (!service) {
    return {};
  }

  return base::ToVector(service->GetUsedContainerIds(),
                        [&](const std::string& id) {
                          return content::StoragePartitionConfig::Create(
                              profile, kContainersStoragePartitionDomain, id,
                              profile->IsOffTheRecord());
                        });
}

}  // namespace containers
