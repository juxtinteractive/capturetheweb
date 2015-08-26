// Copyright (c) 2013 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include "include/cef_app.h"
#import "include/cef_application_mac.h"
#include "cefclient/browser/client_app_browser.h"
#include "cefclient/browser/main_context_impl.h"
#include "cefclient/browser/main_message_loop_std.h"
#include "cefclient/browser/resource.h"
#include "cefclient/browser/root_window.h"
#include "cefclient/browser/test_runner.h"

// Start includes for OSC
#include <pthread.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <osc/OscReceivedElements.h>
#include <osc/OscPrintReceivedElements.h>
#include <osc/OscPacketListener.h>
#include <ip/UdpSocket.h>
// End includes for OSC


// These headers are needed in order build tasks that can be passed to threads
// see resizeIt(int size)
#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"


namespace {

void AddMenuItem(NSMenu *menu, NSString* label, int idval) {
  NSMenuItem* item = [menu addItemWithTitle:label
                                     action:@selector(menuItemSelected:)
                              keyEquivalent:@""];
  [item setTag:idval];
}

}  // namespace

// Receives notifications from the application. Will delete itself when done.
@interface ClientAppDelegate : NSObject<NSApplicationDelegate> {
 @private
  bool with_osr_;
}

- (id)initWithOsr:(bool)with_osr;
- (void)createApplication:(id)object;
- (void)tryToTerminateApplication:(NSApplication*)app;
- (IBAction)menuItemSelected:(id)sender;
@end

// Provide the CefAppProtocol implementation required by CEF.
@interface ClientApplication : NSApplication<CefAppProtocol> {
 @private
  BOOL handlingSendEvent_;
}
@end

@implementation ClientApplication

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event {
  CefScopedSendingEvent sendingEventScoper;
  [super sendEvent:event];
}

// |-terminate:| is the entry point for orderly "quit" operations in Cocoa. This
// includes the application menu's quit menu item and keyboard equivalent, the
// application's dock icon menu's quit menu item, "quit" (not "force quit") in
// the Activity Monitor, and quits triggered by user logout and system restart
// and shutdown.
//
// The default |-terminate:| implementation ends the process by calling exit(),
// and thus never leaves the main run loop. This is unsuitable for Chromium
// since Chromium depends on leaving the main run loop to perform an orderly
// shutdown. We support the normal |-terminate:| interface by overriding the
// default implementation. Our implementation, which is very specific to the
// needs of Chromium, works by asking the application delegate to terminate
// using its |-tryToTerminateApplication:| method.
//
// |-tryToTerminateApplication:| differs from the standard
// |-applicationShouldTerminate:| in that no special event loop is run in the
// case that immediate termination is not possible (e.g., if dialog boxes
// allowing the user to cancel have to be shown). Instead, this method tries to
// close all browsers by calling CloseBrowser(false) via
// ClientHandler::CloseAllBrowsers. Calling CloseBrowser will result in a call
// to ClientHandler::DoClose and execution of |-performClose:| on the NSWindow.
// DoClose sets a flag that is used to differentiate between new close events
// (e.g., user clicked the window close button) and in-progress close events
// (e.g., user approved the close window dialog). The NSWindowDelegate
// |-windowShouldClose:| method checks this flag and either calls
// CloseBrowser(false) in the case of a new close event or destructs the
// NSWindow in the case of an in-progress close event.
// ClientHandler::OnBeforeClose will be called after the CEF NSView hosted in
// the NSWindow is dealloc'ed.
//
// After the final browser window has closed ClientHandler::OnBeforeClose will
// begin actual tear-down of the application by calling CefQuitMessageLoop.
// This ends the NSApplication event loop and execution then returns to the
// main() function for cleanup before application termination.
//
// The standard |-applicationShouldTerminate:| is not supported, and code paths
// leading to it must be redirected.
- (void)terminate:(id)sender {
  ClientAppDelegate* delegate = static_cast<ClientAppDelegate*>(
      [[NSApplication sharedApplication] delegate]);
  [delegate tryToTerminateApplication:self];
  // Return, don't exit. The application is responsible for exiting on its own.
}
@end

@implementation ClientAppDelegate

- (id)initWithOsr:(bool)with_osr {
  if (self = [super init]) {
    with_osr_ = with_osr;
  }
  return self;
}

// Create the application on the UI thread.
- (void)createApplication:(id)object {
  NSApplication* application = [NSApplication sharedApplication];
  [NSBundle loadNibNamed:@"MainMenu" owner:NSApp];

  // Set the delegate for application events.
  [application setDelegate:self];

  // Add the Tests menu.
  NSMenu* menubar = [application mainMenu];
  NSMenuItem *testItem = [[[NSMenuItem alloc] initWithTitle:@"Tests"
                                                     action:nil
                                              keyEquivalent:@""] autorelease];
  NSMenu *testMenu = [[[NSMenu alloc] initWithTitle:@"Tests"] autorelease];
  AddMenuItem(testMenu, @"Get Text",      ID_TESTS_GETSOURCE);
  AddMenuItem(testMenu, @"Get Source",    ID_TESTS_GETTEXT);
  AddMenuItem(testMenu, @"New Window",    ID_TESTS_WINDOW_NEW);
  AddMenuItem(testMenu, @"Popup Window",  ID_TESTS_WINDOW_POPUP);
  AddMenuItem(testMenu, @"Request",       ID_TESTS_REQUEST);
  AddMenuItem(testMenu, @"Plugin Info",   ID_TESTS_PLUGIN_INFO);
  AddMenuItem(testMenu, @"Zoom In",       ID_TESTS_ZOOM_IN);
  AddMenuItem(testMenu, @"Zoom Out",      ID_TESTS_ZOOM_OUT);
  AddMenuItem(testMenu, @"Zoom Reset",    ID_TESTS_ZOOM_RESET);
  if (with_osr_) {
    AddMenuItem(testMenu, @"Set FPS",          ID_TESTS_OSR_FPS);
    AddMenuItem(testMenu, @"Set Scale Factor", ID_TESTS_OSR_DSF);
  }
  AddMenuItem(testMenu, @"Begin Tracing", ID_TESTS_TRACING_BEGIN);
  AddMenuItem(testMenu, @"End Tracing",   ID_TESTS_TRACING_END);
  AddMenuItem(testMenu, @"Print",         ID_TESTS_PRINT);
  AddMenuItem(testMenu, @"Print to PDF",  ID_TESTS_PRINT_TO_PDF);
  AddMenuItem(testMenu, @"Other Tests",   ID_TESTS_OTHER_TESTS);
  [testItem setSubmenu:testMenu];
  [menubar addItem:testItem];

  // Create the first window.
  client::MainContext::Get()->GetRootWindowManager()->CreateRootWindow(
      true,             // Show controls.
      with_osr_,        // Use off-screen rendering.
      CefRect(),        // Use default system size.
      std::string());   // Use default URL.
}

- (void)tryToTerminateApplication:(NSApplication*)app {
  client::MainContext::Get()->GetRootWindowManager()->CloseAllWindows(false);
}

- (IBAction)menuItemSelected:(id)sender {
  // Retrieve the active RootWindow.
  NSWindow* key_window = [[NSApplication sharedApplication] keyWindow];
  if (!key_window)
    return;

  scoped_refptr<client::RootWindow> root_window =
      client::RootWindow::GetForNSWindow(key_window);

  CefRefPtr<CefBrowser> browser = root_window->GetBrowser();
  if (browser.get()) {
    NSMenuItem *item = (NSMenuItem*)sender;
    client::test_runner::RunTest(browser, [item tag]);
  }
}

- (NSApplicationTerminateReply)applicationShouldTerminate:
      (NSApplication *)sender {
  return NSTerminateNow;
}

@end

namespace client {
namespace {

CefRefPtr<CefBrowser> getBrowser() {
  // Retrieve the active RootWindow.
  NSWindow* key_window = [[ClientApplication sharedApplication] keyWindow];
  if (!key_window) {
    std::cout << "football" << std::endl;
    return NULL;
  }

  scoped_refptr<client::RootWindow> root_window =
      client::RootWindow::GetForNSWindow(key_window);

  CefRefPtr<CefBrowser> browser = root_window->GetBrowser();

  return browser;
}

void resizeIt(int width, int height) {
  // We are not allowed to access the main window handle unless we're on the CEF UI thread
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&resizeIt, width, height));
    return;
  }

  CefRefPtr<CefBrowser> browser = getBrowser();
  if(!browser.get())
    return;

  NSWindow *window = browser->GetHost()->GetWindowHandle().window;

  NSRect newFrame = window.frame;
  newFrame.size.width = width > 0 ? width : 1;
  newFrame.size.height = height > 0 ? height : 1;

  [window setFrame:newFrame display:YES];

}

void gotoURL(const char *msgUrl) {

  CefRefPtr<CefBrowser> browser = getBrowser();
  if(!browser.get())
    return;

  NSString *url = [NSString stringWithUTF8String:msgUrl];

  // if it doesn't already have a prefix, add http. If we can't parse it,
  // just don't bother rather than making things worse.
  NSURL* tempUrl = [NSURL URLWithString:url];
  if (tempUrl && ![tempUrl scheme])
    url = [@"http://" stringByAppendingString:url];

  std::string urlStr = [url UTF8String];
  browser->GetMainFrame()->LoadURL(urlStr);

}

void moveMouse(float val) {
  CefRefPtr<CefBrowser> browser = getBrowser();
  std::cout << "check" << std::endl;
  if(!browser.get())
    return;
  std::cout << "were in" << std::endl;

  NSSize winSize = browser->GetHost()->GetWindowHandle().frame.size;

  CefMouseEvent mouseEvent;
  mouseEvent.x = (int)(val * winSize.width);
  mouseEvent.y = (int)(val * winSize.height);

  browser->GetHost()->SendMouseMoveEvent(mouseEvent, false);

}


class OscDumpPacketListener : public osc::OscPacketListener{
protected:
  virtual void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) {
    (void) remoteEndpoint; // suppress unused parameter warning


    try {
      osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();

      if(arg->IsString()) {

        const char *msgUrl = arg->AsString();
        gotoURL(msgUrl);

      } else if(arg->IsInt32()) {

        int size = arg->AsInt32();
        resizeIt(size, size);

      } else if(arg->IsFloat()) {

        float val = arg->AsFloat();
        moveMouse(val);

      }

    } catch (osc::Exception& e) {
      std::cout << "Error processing OSC message: " << m.AddressPattern() << ":" << e.what() << std::endl;
    }
  }
};


void *startOscListener(void *threadId) {
  int port = 7000;

  OscDumpPacketListener listener;
  UdpListeningReceiveSocket s(
                              IpEndpointName( IpEndpointName::ANY_ADDRESS, port ),
                              &listener );

  s.RunUntilSigInt();

  pthread_exit(NULL);
}

int RunMain(int argc, char* argv[]) {
  CefMainArgs main_args(argc, argv);

  // Initialize the AutoRelease pool.
  NSAutoreleasePool* autopool = [[NSAutoreleasePool alloc] init];

  // Initialize the ClientApplication instance.
  [ClientApplication sharedApplication];

  // Parse command-line arguments.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromArgv(argc, argv);

  // Create a ClientApp of the correct type.
  CefRefPtr<CefApp> app;
  ClientApp::ProcessType process_type = ClientApp::GetProcessType(command_line);
  if (process_type == ClientApp::BrowserProcess)
    app = new ClientAppBrowser();

  // Create the main context object.
  scoped_ptr<MainContextImpl> context(new MainContextImpl(command_line, true));

  CefSettings settings;

  // Populate the settings based on command line arguments.
  context->PopulateSettings(&settings);

  // Create the main message loop object.
  scoped_ptr<MainMessageLoop> message_loop(new MainMessageLoopStd);

  // Initialize CEF.
  context->Initialize(main_args, settings, app, NULL);

  // Register scheme handlers.
  test_runner::RegisterSchemeHandlers();

  // Create the application delegate and window.
  ClientAppDelegate* delegate = [[ClientAppDelegate alloc] initWithOsr:true];
      // initWithOsr:settings.windowless_rendering_enabled ? true : false];
  [delegate performSelectorOnMainThread:@selector(createApplication:)
                             withObject:nil
                          waitUntilDone:NO];






  pthread_t threads[1];
  pthread_create(&threads[0], NULL, startOscListener, 0);






  // Run the message loop. This will block until Quit() is called.
  int result = message_loop->Run();

  // Shut down CEF.
  context->Shutdown();

  // Release objects in reverse order of creation.
  [delegate release];
  message_loop.reset();
  context.reset();
  [autopool release];

  return result;
}

}  // namespace
}  // namespace client


// Program entry point function.
int main(int argc, char* argv[]) {
  return client::RunMain(argc, argv);
}
