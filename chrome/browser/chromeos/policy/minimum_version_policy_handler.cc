// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/minimum_version_policy_handler.h"

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/version.h"
#include "chromeos/settings/cros_settings_names.h"
#include "chromeos/settings/cros_settings_provider.h"
#include "components/version_info/version_info.h"

namespace policy {

MinimumVersionPolicyHandler::MinimumVersionPolicyHandler(
    chromeos::CrosSettings* cros_settings)
    : cros_settings_(cros_settings), weak_factory_(this) {
  policy_subscription_ = cros_settings_->AddSettingsObserver(
      chromeos::kMinimumRequiredChromeVersion,
      base::Bind(&MinimumVersionPolicyHandler::OnPolicyChanged,
                 weak_factory_.GetWeakPtr()));
  // Fire it once so we're sure we get an invocation on startup.
  OnPolicyChanged();
}

MinimumVersionPolicyHandler::~MinimumVersionPolicyHandler() = default;

void MinimumVersionPolicyHandler::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MinimumVersionPolicyHandler::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MinimumVersionPolicyHandler::NotifyMinimumVersionStateChanged() {
  for (auto& observer : observers_)
    observer.OnMinimumVersionStateChanged();
}

void MinimumVersionPolicyHandler::OnPolicyChanged() {
  chromeos::CrosSettingsProvider::TrustedStatus status =
      cros_settings_->PrepareTrustedValues(
          base::Bind(&MinimumVersionPolicyHandler::OnPolicyChanged,
                     weak_factory_.GetWeakPtr()));
  if (status != chromeos::CrosSettingsProvider::TRUSTED)
    return;

  requirements_met_ = true;
  std::string min_chrome_version_string;
  cros_settings_->GetString(chromeos::kMinimumRequiredChromeVersion,
                            &min_chrome_version_string);
  if (min_chrome_version_string.empty())
    return;

  const base::Version required_version(min_chrome_version_string);
  if (!required_version.IsValid()) {
    LOG(ERROR) << "Version supplied by policy is not a valid version "
               << required_version;
    return;
  }

  const base::Version current_version(version_info::GetVersionNumber());
  requirements_met_ = current_version.CompareTo(required_version) >= 0;
  NotifyMinimumVersionStateChanged();
}

}  // namespace policy
