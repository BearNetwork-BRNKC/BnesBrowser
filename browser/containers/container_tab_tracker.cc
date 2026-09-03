// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "BnesBrowser/browser/containers/container_tab_tracker.h"

#include "base/memory/ptr_util.h"
#include "BnesBrowser/browser/containers/containers_service_factory.h"
#include "BnesBrowser/components/containers/content/browser/containers_web_contents_user_data.h"
#include "BnesBrowser/components/containers/content/browser/storage_partition_utils.h"
#include "BnesBrowser/components/containers/core/browser/containers_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace containers {

// static
std::unique_ptr<ContainerTabTracker> ContainerTabTracker::MaybeCreate(
    tabs::TabInterface& tab) {
  content::WebContents* const contents = tab.GetContents();
  CHECK(contents);
  if (!ContainersServiceFactory::GetForProfile(
          Profile::FromBrowserContext(contents->GetBrowserContext()))) {
    return nullptr;
  }
  return base::WrapUnique(new ContainerTabTracker(tab));
}

ContainerTabTracker::ContainerTabTracker(tabs::TabInterface& tab)
    : tabs::ContentsObservingTabFeature(tab) {}

ContainerTabTracker::~ContainerTabTracker() = default;

void ContainerTabTracker::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  ContainersWebContentsUserData::CopyContainerStateForDiscardedContents(
      old_contents, new_contents);
  tracked_ = false;
  tabs::ContentsObservingTabFeature::OnDiscardContents(tab, old_contents,
                                                       new_contents);
}

void ContainerTabTracker::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (tracked_) {
    return;
  }
  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  tracked_ = true;

  const std::string container_id =
      GetContainerIdForWebContents(navigation_handle->GetWebContents());
  if (container_id.empty()) {
    return;
  }

  auto* service =
      ContainersServiceFactory::GetForProfile(Profile::FromBrowserContext(
          navigation_handle->GetWebContents()->GetBrowserContext()));
  CHECK(service);

  service->MarkContainerUsed(container_id);
}

}  // namespace containers
