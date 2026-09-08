/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_PROFILE_PREFS_H_
#define BRAVE_BROWSER_BRAVE_PROFILE_PREFS_H_

#include "base/files/file_path.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;

namespace brave {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

void MigrateObsoleteBraveProfilePrefsBeforeChromium(PrefService* profile_prefs);
void MigrateObsoleteBraveProfilePrefsAfterChromium(PrefService* profile_prefs,
                                                    const base::FilePath& profile_path);

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_PROFILE_PREFS_H_
