// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_WINDOW_TEST_H_
#define CEF_TESTS_CEFCLIENT_WINDOW_TEST_H_
#pragma once

#include "cefclient/test_runner.h"

namespace client {
namespace window_test {

/// Handler creation.
void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers);

// Fit |window| inside |display|. Coordinates are relative to the upper-left
// corner of the display.
void ModifyBounds(const CefRect& display, CefRect& window);

// Platform implementations.
void SetPos(CefRefPtr<CefBrowser> browser,
            int x, int y, int width, int height);
void Minimize(CefRefPtr<CefBrowser> browser);
void Maximize(CefRefPtr<CefBrowser> browser);
void Restore(CefRefPtr<CefBrowser> browser);

}  // namespace window_test
}  // namespace client

#endif  // CEF_TESTS_CEFCLIENT_WINDOW_TEST_H_
