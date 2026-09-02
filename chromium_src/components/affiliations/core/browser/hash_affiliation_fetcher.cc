/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Chromium 152's HashAffiliationFetcher has no CompleteStubbedRequest()
// declaration. A previous BNES stub added an out-of-line definition without
// a header injection (same class of error as FeatureList::GetCompileTimeFeatureState).
// Production builds compile the upstream implementation; the unittest overlay
// still describes the intended stub behavior.

#include <components/affiliations/core/browser/hash_affiliation_fetcher.cc>
