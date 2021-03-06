// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/message_center_notification_manager.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/notifications/profile_notification.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/message_center_types.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notifier_id.h"
#include "ui/message_center/public/cpp/message_center_constants.h"
#include "ui/message_center/ui_controller.h"

#if !defined(OS_CHROMEOS)
#include "chrome/browser/notifications/fullscreen_notification_blocker.h"
#include "chrome/browser/notifications/screen_lock_notification_blocker.h"
#endif

using message_center::NotifierId;

MessageCenterNotificationManager::MessageCenterNotificationManager(
    message_center::MessageCenter* message_center)
    : message_center_(message_center),
      system_observer_(this),
      stats_collector_(message_center) {
  message_center_->AddObserver(this);

#if !defined(OS_CHROMEOS)
  blockers_.push_back(
      std::make_unique<ScreenLockNotificationBlocker>(message_center));
  blockers_.push_back(
      std::make_unique<FullscreenNotificationBlocker>(message_center));
#endif

#if defined(OS_WIN) || defined(OS_MACOSX) \
  || (defined(OS_LINUX) && !defined(OS_CHROMEOS))
  // On Windows, Linux and Mac, the notification manager owns the tray icon and
  // views.Other platforms have global ownership and Create will return NULL.
  tray_.reset(CreateUiDelegate());
#endif
}

MessageCenterNotificationManager::~MessageCenterNotificationManager() {
  message_center_->RemoveObserver(this);

  profile_notifications_.clear();
}

////////////////////////////////////////////////////////////////////////////////
// NotificationUIManager

void MessageCenterNotificationManager::Add(
    const message_center::Notification& notification,
    Profile* profile) {
  // We won't have time to process and act on this notification.
  if (is_shutdown_started_)
    return;

  if (Update(notification, profile))
    return;

  auto profile_notification_ptr =
      std::make_unique<ProfileNotification>(profile, notification);
  ProfileNotification* profile_notification = profile_notification_ptr.get();

  // WARNING: You MUST use AddProfileNotification or update the message center
  // via the notification within a ProfileNotification object or the profile ID
  // will not be correctly set for ChromeOS.
  // Takes ownership of profile_notification.
  AddProfileNotification(std::move(profile_notification_ptr));

  message_center_->AddNotification(
      std::make_unique<message_center::Notification>(
          profile_notification->notification()));
}

bool MessageCenterNotificationManager::Update(
    const message_center::Notification& notification,
    Profile* profile) {
  const std::string profile_id = ProfileNotification::GetProfileNotificationId(
      notification.id(), NotificationUIManager::GetProfileID(profile));
  for (auto iter = profile_notifications_.begin();
       iter != profile_notifications_.end(); ++iter) {
    ProfileNotification* old_notification = (*iter).second.get();
    if (old_notification->notification().id() != profile_id)
      continue;

    // The ID should uniquely identify the notification, but as a sanity check
    // make sure we got the right origin URL and profile.
    DCHECK_EQ(old_notification->notification().origin_url(),
              notification.origin_url());
    DCHECK_EQ(old_notification->profile_id(),
              NotificationUIManager::GetProfileID(profile));

    // Changing the type from non-progress to progress does not count towards
    // the immediate update allowed in the message center.
    std::string old_id = old_notification->notification().id();

    // Add/remove notification in the local list but just update the same
    // one in MessageCenter.
    auto new_notification =
        std::make_unique<ProfileNotification>(profile, notification);
    const message_center::Notification& notification =
        new_notification->notification();
    // Delete the old one after the new one is created to ensure we don't run
    // out of KeepAlives.
    profile_notifications_.erase(old_id);
    profile_notifications_[notification.id()] = std::move(new_notification);

    // TODO(liyanhou): Add routing updated notifications to alternative
    // providers.

    // WARNING: You MUST use AddProfileNotification or update the message
    // center via the notification within a ProfileNotification object or the
    // profile ID will not be correctly set for ChromeOS.
    message_center_->UpdateNotification(
        old_id, std::make_unique<message_center::Notification>(notification));
    return true;
  }

  return false;
}

const message_center::Notification* MessageCenterNotificationManager::FindById(
    const std::string& id,
    ProfileID profile_id) const {
  std::string profile_notification_id =
      ProfileNotification::GetProfileNotificationId(id, profile_id);
  auto iter = profile_notifications_.find(profile_notification_id);
  if (iter == profile_notifications_.end())
    return nullptr;
  return &(iter->second->notification());
}

bool MessageCenterNotificationManager::CancelById(const std::string& id,
                                                  ProfileID profile_id) {
  std::string profile_notification_id =
      ProfileNotification::GetProfileNotificationId(id, profile_id);
  // See if this ID hasn't been shown yet.
  // If it has been shown, remove it.
  auto iter = profile_notifications_.find(profile_notification_id);
  if (iter == profile_notifications_.end())
    return false;

  RemoveProfileNotification(iter->first);
  message_center_->RemoveNotification(profile_notification_id,
                                      /* by_user */ false);
  return true;
}

std::set<std::string> MessageCenterNotificationManager::GetAllIdsByProfile(
    ProfileID profile_id) {
  std::set<std::string> original_ids;
  for (const auto& pair : profile_notifications_) {
    if (pair.second->profile_id() == profile_id)
      original_ids.insert(pair.second->original_id());
  }

  return original_ids;
}

bool MessageCenterNotificationManager::CancelAllBySourceOrigin(
    const GURL& source) {
  // Same pattern as CancelById, but more complicated than the above
  // because there may be multiple notifications from the same source.
  bool removed = false;

  for (auto loopiter = profile_notifications_.begin();
       loopiter != profile_notifications_.end();) {
    auto curiter = loopiter++;
    if ((*curiter).second->notification().origin_url() == source) {
      const std::string id = curiter->first;
      RemoveProfileNotification(id);
      message_center_->RemoveNotification(id, /* by_user */ false);
      removed = true;
    }
  }
  return removed;
}

bool MessageCenterNotificationManager::CancelAllByProfile(
    ProfileID profile_id) {
  // Same pattern as CancelAllBySourceOrigin.
  bool removed = false;

  for (auto loopiter = profile_notifications_.begin();
       loopiter != profile_notifications_.end();) {
    auto curiter = loopiter++;
    if (profile_id == (*curiter).second->profile_id()) {
      const std::string id = curiter->first;
      RemoveProfileNotification(id);
      message_center_->RemoveNotification(id, /* by_user */ false);
      removed = true;
    }
  }
  return removed;
}

void MessageCenterNotificationManager::CancelAll() {
  message_center_->RemoveAllNotifications(
      false /* by_user */, message_center::MessageCenter::RemoveType::ALL);
}

void MessageCenterNotificationManager::StartShutdown() {
  is_shutdown_started_ = true;
  CancelAll();
}

////////////////////////////////////////////////////////////////////////////////
// MessageCenter::Observer
void MessageCenterNotificationManager::OnNotificationRemoved(
    const std::string& id,
    bool by_user) {
  RemoveProfileNotification(id);
}

void MessageCenterNotificationManager::SetUiDelegateForTest(
    message_center::UiDelegate* delegate) {
  tray_.reset(delegate);
}

std::string
MessageCenterNotificationManager::GetMessageCenterNotificationIdForTest(
    const std::string& id,
    Profile* profile) {
  return ProfileNotification::GetProfileNotificationId(id,
                                                       GetProfileID(profile));
}

////////////////////////////////////////////////////////////////////////////////
// private

void MessageCenterNotificationManager::AddProfileNotification(
    std::unique_ptr<ProfileNotification> profile_notification) {
  const message_center::Notification& notification =
      profile_notification->notification();
  std::string id = notification.id();
  // Notification ids should be unique.
  DCHECK(profile_notifications_.find(id) == profile_notifications_.end());
  profile_notifications_[id] = std::move(profile_notification);
}

void MessageCenterNotificationManager::RemoveProfileNotification(
    const std::string& notification_id) {
  auto it = profile_notifications_.find(notification_id);
  if (it == profile_notifications_.end())
    return;

  // Delay destruction of the ProfileNotification until current task is
  // completed. This must be done because this ProfileNotification might have
  // the one ScopedKeepAlive object that was keeping the browser alive, and
  // destroying it would result in:
  // a) A reentrant call to this class. Because every method in this class
  //   touches |profile_notifications_|, |profile_notifications_| must always
  //   be in a self-consistent state in moments where re-entrance might happen.
  // b) A crash like https://crbug.com/649971 because it can trigger
  //    shutdown process while we're still inside the call stack from UI
  //    framework.
  content::BrowserThread::DeleteSoon(content::BrowserThread::UI, FROM_HERE,
                                     it->second.release());
  profile_notifications_.erase(it);
}

ProfileNotification* MessageCenterNotificationManager::FindProfileNotification(
    const std::string& id) const {
  auto iter = profile_notifications_.find(id);
  if (iter == profile_notifications_.end())
    return nullptr;

  return (*iter).second.get();
}
