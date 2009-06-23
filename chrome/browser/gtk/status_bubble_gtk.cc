// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/status_bubble_gtk.h"

#include <gtk/gtk.h>

#include "base/gfx/gtk_util.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "chrome/browser/gtk/slide_animator_gtk.h"
#include "chrome/common/gtk_util.h"
#include "googleurl/src/gurl.h"

namespace {

const GdkColor kTextColor = GDK_COLOR_RGB(100, 100, 100);
const GdkColor kBackgroundColor = GDK_COLOR_RGB(0xe6, 0xed, 0xf4);
const GdkColor kFrameBorderColor = GDK_COLOR_RGB(0xbe, 0xc8, 0xd4);

// Inner padding between the border and the text label.
const int kInternalTopBottomPadding = 1;
const int kInternalLeftRightPadding = 2;

// Border of color kFrameBorderColor around the status bubble.
const int kBorderPadding = 1;

// Milliseconds before we hide the status bubble widget when you mouseout.
static const int kHideDelay = 250;

}  // namespace

StatusBubbleGtk::StatusBubbleGtk()
    : timer_factory_(this) {
  InitWidgets();
}

StatusBubbleGtk::~StatusBubbleGtk() {
  container_.Destroy();
}

void StatusBubbleGtk::SetStatus(const std::wstring& status_text_wide) {
  std::string status_text = WideToUTF8(status_text_wide);
  if (status_text_ == status_text)
    return;

  status_text_ = status_text;
  if (!status_text_.empty()) {
    SetStatusTextTo(status_text_);
  } else if (!url_text_.empty()) {
    SetStatusTextTo(url_text_);
  } else {
    SetStatusTextTo(std::string());
  }
}

void StatusBubbleGtk::SetURL(const GURL& url, const std::wstring& languages) {
  // If we want to clear a displayed URL but there is a status still to
  // display, display that status instead.
  if (url.is_empty() && !status_text_.empty()) {
    url_text_ = std::string();
    SetStatusTextTo(status_text_);
    return;
  }

  // TODO(erg): We probably want to elide the text from the GURL object.
  url_text_ = url.possibly_invalid_spec();

  SetStatusTextTo(url_text_);
}

void StatusBubbleGtk::Show() {
  // If we were going to hide, stop.
  timer_factory_.RevokeAll();

  gtk_widget_show_all(container_.get());

  if (container_->window)
    gdk_window_raise(container_->window);
}

void StatusBubbleGtk::Hide() {
  gtk_widget_hide_all(container_.get());
}

void StatusBubbleGtk::SetStatusTextTo(const std::string& status_utf8) {
  if (status_utf8.empty()) {
    HideInASecond();
  } else {
    gtk_label_set_text(GTK_LABEL(label_), status_utf8.c_str());
    Show();
  }
}

void StatusBubbleGtk::HideInASecond() {
  if (!timer_factory_.empty())
    timer_factory_.RevokeAll();

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
      timer_factory_.NewRunnableMethod(&StatusBubbleGtk::Hide),
      kHideDelay);
}

void StatusBubbleGtk::MouseMoved() {
  // We can't do that fancy sliding behaviour where the status bubble slides
  // out of the window because the window manager gets in the way. So totally
  // ignore this message for now.
  //
  // TODO(erg): At least get some sliding behaviour so that it slides out of
  // the way to hide the status bubble on mouseover.
}

void StatusBubbleGtk::InitWidgets() {
  label_ = gtk_label_new(NULL);
  gtk_widget_modify_fg(label_, GTK_STATE_NORMAL, &kTextColor);

  GtkWidget* padding = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(padding),
      kInternalTopBottomPadding, kInternalTopBottomPadding,
      kInternalLeftRightPadding, kInternalLeftRightPadding);
  gtk_container_add(GTK_CONTAINER(padding), label_);

  GtkWidget* bg_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(bg_box), padding);
  gtk_widget_modify_bg(bg_box, GTK_STATE_NORMAL, &kBackgroundColor);

  container_.Own(gtk_util::CreateGtkBorderBin(bg_box, &kFrameBorderColor,
          kBorderPadding, kBorderPadding, kBorderPadding, kBorderPadding));
  gtk_widget_set_name(container_.get(), "status-bubble");
  gtk_widget_set_app_paintable(container_.get(), TRUE);
}
