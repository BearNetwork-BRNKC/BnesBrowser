/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "BnesBrowser/components/brave_origin/features.h"

#include "base/feature_list.h"

namespace brave_origin::features {

// BNES: Brave Origin is a paid variant of Brave (bundled adblock + update
// support). BNES is fully independent and does not rely on Brave services or
// auth, so this feature is disabled. Disabling it also makes
// IsBraveOriginPurchased() always return false, which hides the "Brave Origin"
// purchase card in brave://settings/system.
BASE_FEATURE(kBraveOrigin, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_origin::features
