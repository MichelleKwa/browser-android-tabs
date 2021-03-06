// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_UTIL_NAMED_GUIDE_H_
#define IOS_CHROME_BROWSER_UI_UTIL_NAMED_GUIDE_H_

#import <UIKit/UIKit.h>

typedef NSString GuideName;

//////////////////////////////////////////
// The list of well-known UILayoutGuides.  When adding a new guide to the app,
// create a constant for it below.

// A guide that is constrained to match the frame of the omnibox.
extern GuideName* const kOmniboxGuide;
// A guide that is constrained to match the frame of the back button's image.
extern GuideName* const kBackButtonGuide;
// A guide that is constrained to match the frame of the forward button's image.
extern GuideName* const kForwardButtonGuide;
// A guide that is constrained to match the frame of the TabSwitcher button's
// image.
extern GuideName* const kTabSwitcherGuide;
// A guide that is constrained to match the frame of the ToolsMenu button.
extern GuideName* const kToolsMenuGuide;

//
//////////////////////////////////////////

// Returns the UILayoutGuide with the given |name|, if one is attached to |view|
// or one of |view|'s ancestors.  If no guide is found, returns nil.
UILayoutGuide* FindNamedGuide(GuideName* name, UIView* view);

// Creates and attaches a new UILayoutGuide to the given |view| and makes it
// globally accessible under the given |name|.  It is an error to add a guide
// with the same name as an existing guide.  Returns the newly-created guide.
UILayoutGuide* AddNamedGuide(GuideName* name, UIView* view);

// Adds constraints such as the layoutGuide with |guideName| matches the |view|.
// The layout guide has to be owned by |view| or one of its superview.
void ConstrainNamedGuideToView(GuideName* guideName, UIView* view);

#endif  // IOS_CHROME_BROWSER_UI_UTIL_NAMED_GUIDE_H_
