// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of this file was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "BnesBrowser/components/containers/core/browser/temporary_container.h"

#include <array>
#include <optional>
#include <string>

#include "base/check.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/uuid.h"
#include "BnesBrowser/components/containers/core/mojom/containers.mojom-shared.h"
#include "BnesBrowser/components/containers/core/mojom/containers.mojom.h"
#include "BnesBrowser/ui/color/nala/nala_color_id.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_manager.h"

namespace containers {
namespace {

constexpr char kContainerNameChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Generates a random temporary container name. The first character is title
// cased.
std::string GenerateTemporaryContainerName() {
  const int kNameLength = 8;
  const int kCharCount = static_cast<int>(std::size(kContainerNameChars));
  std::string name;
  name.reserve(kNameLength);
  for (int i = 0; i < kNameLength; ++i) {
    int idx = base::RandIntInclusive(0, kCharCount - 1);
    name.push_back(kContainerNameChars[idx]);
  }
  name[0] = base::ToUpperASCII(name[0]);
  return name;
}

// Picks a random container icon from the available icons.
mojom::Icon PickTemporaryContainerIcon() {
  static constexpr auto kIconIds = std::to_array({
      mojom::Icon::kPersonal,
      mojom::Icon::kWork,
      mojom::Icon::kShopping,
      mojom::Icon::kSocial,
      mojom::Icon::kEvents,
      mojom::Icon::kBanking,
      mojom::Icon::kStar,
      mojom::Icon::kTravel,
      mojom::Icon::kSchool,
      mojom::Icon::kPrivate,
      mojom::Icon::kMessaging,
  });
  return base::RandomChoice(kIconIds);
}

// Picks a random container background color from the available colors.
SkColor PickTemporaryContainerBackground() {
  const auto* color_provider =
      ui::ColorProviderManager::Get().GetColorProviderFor(
          ui::ColorProviderKey());
  CHECK(color_provider);

  // Keep in sync with
  // brave/browser/resources/settings/brave_content_page/background_colors.ts.
  static constexpr auto kBackgroundColorIds = std::to_array({
      nala::kColorPrimitiveRed60,
      nala::kColorPrimitiveOrange60,
      nala::kColorPrimitiveYellow60,
      nala::kColorPrimitiveGreen60,
      nala::kColorPrimitiveTeal60,
      nala::kColorPrimitiveBlue60,
      nala::kColorPrimitivePurple60,
      nala::kColorPrimitivePink60,
  });
  return color_provider->GetColor(base::RandomChoice(kBackgroundColorIds));
}

}  // namespace

bool IsTemporaryContainerId(std::string_view container_id) {
  // Temporary container ids are prefixed with "t-" and should have the actual
  // id after the prefix.
  return container_id.size() > kTemporaryContainerIdPrefix.size() &&
         container_id.starts_with(kTemporaryContainerIdPrefix);
}

mojom::ContainerPtr CreateTemporaryContainer(
    std::optional<std::string_view> name) {
  return mojom::Container::New(
      base::StrCat({kTemporaryContainerIdPrefix,
                    base::Uuid::GenerateRandomV4().AsLowercaseString()}),
      name ? std::string(*name) : GenerateTemporaryContainerName(),
      PickTemporaryContainerIcon(), PickTemporaryContainerBackground());
}

}  // namespace containers
