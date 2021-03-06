// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/prerender/prerender_contents.h"
#include "chrome/browser/prerender/prerender_histograms.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "extensions/features/features.h"

ChromeNavigationUIData::ChromeNavigationUIData() {}

ChromeNavigationUIData::ChromeNavigationUIData(
    content::NavigationHandle* navigation_handle) {
  auto* web_contents = navigation_handle->GetWebContents();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  SessionTabHelper* session_tab_helper =
      SessionTabHelper::FromWebContents(web_contents);
  int tab_id = session_tab_helper ? session_tab_helper->session_id().id() : -1;
  int window_id =
      session_tab_helper ? session_tab_helper->window_id().id() : -1;
  extension_data_ = base::MakeUnique<extensions::ExtensionNavigationUIData>(
      navigation_handle, tab_id, window_id);
#endif

  auto* prerender_contents =
      prerender::PrerenderContents::FromWebContents(web_contents);
  if (prerender_contents) {
    prerender_mode_ = prerender_contents->prerender_mode();
    prerender_histogram_prefix_ =
        prerender::PrerenderHistograms::GetHistogramPrefix(
            prerender_contents->origin());
  }
}

ChromeNavigationUIData::~ChromeNavigationUIData() {}

std::unique_ptr<content::NavigationUIData> ChromeNavigationUIData::Clone()
    const {
  std::unique_ptr<ChromeNavigationUIData> copy =
      base::MakeUnique<ChromeNavigationUIData>();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (extension_data_)
    copy->SetExtensionNavigationUIData(extension_data_->DeepCopy());
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  if (offline_page_data_)
    copy->SetOfflinePageNavigationUIData(offline_page_data_->DeepCopy());
#endif

  copy->prerender_mode_ = prerender_mode_;
  copy->prerender_histogram_prefix_ = prerender_histogram_prefix_;

  return std::move(copy);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
void ChromeNavigationUIData::SetExtensionNavigationUIData(
    std::unique_ptr<extensions::ExtensionNavigationUIData> extension_data) {
  extension_data_ = std::move(extension_data);
}
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
void ChromeNavigationUIData::SetOfflinePageNavigationUIData(
    std::unique_ptr<offline_pages::OfflinePageNavigationUIData>
        offline_page_data) {
  offline_page_data_ = std::move(offline_page_data);
}
#endif
