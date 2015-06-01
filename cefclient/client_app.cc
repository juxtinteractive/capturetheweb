// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

// This file is shared by cefclient and cef_unittests so don't include using
// a qualified path.
#include "client_app.h"  // NOLINT(build/include)

#include <string>
#include <sstream>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "include/wrapper/cef_helpers.h"

namespace client {

ClientApp::ClientApp() {
}

void ClientApp::OnRegisterCustomSchemes(
    CefRefPtr<CefSchemeRegistrar> registrar) {
  // Default schemes that support cookies.
  cookieable_schemes_.push_back("http");
  cookieable_schemes_.push_back("https");

  RegisterCustomSchemes(registrar, cookieable_schemes_);
}

void ClientApp::OnContextInitialized() {
  CreateBrowserDelegates(browser_delegates_);

  // Register cookieable schemes with the global cookie manager.
  CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager();
  DCHECK(manager.get());
  manager->SetSupportedSchemes(cookieable_schemes_);

  print_handler_ = CreatePrintHandler();

  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnContextInitialized(this);
}

void ClientApp::OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) {
  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnBeforeChildProcessLaunch(this, command_line);
}

void ClientApp::OnRenderProcessThreadCreated(
    CefRefPtr<CefListValue> extra_info) {
  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnRenderProcessThreadCreated(this, extra_info);
}

void ClientApp::OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info) {
  CreateRenderDelegates(render_delegates_);

  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnRenderThreadCreated(this, extra_info);
}

void ClientApp::OnWebKitInitialized() {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnWebKitInitialized(this);
}

void ClientApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnBrowserCreated(this, browser);
}

void ClientApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnBrowserDestroyed(this, browser);
}

CefRefPtr<CefLoadHandler> ClientApp::GetLoadHandler() {
  CefRefPtr<CefLoadHandler> load_handler;
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end() && !load_handler.get(); ++it)
    load_handler = (*it)->GetLoadHandler(this);

  return load_handler;
}

bool ClientApp::OnBeforeNavigation(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefRequest> request,
                                   NavigationType navigation_type,
                                   bool is_redirect) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    if ((*it)->OnBeforeNavigation(this, browser, frame, request,
                                  navigation_type, is_redirect)) {
      return true;
    }
  }

  return false;
}
  
  
class MyV8Handler : public CefV8Handler {
public:
  MyV8Handler() {}
  
  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) OVERRIDE {
    if (name == "send") {

      if(arguments.size() == 1 && arguments[0]->IsString()) {
        
        std::string message = arguments[0]->GetStringValue().ToString();
  
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
        
        
        // Create the message object.
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("my_message");
        
        // Retrieve the argument list object.
        CefRefPtr<CefListValue> args = msg->GetArgumentList();
        
        // Populate the argument values.
        args->SetString(0, message);
        
        // Send the process message to the render process.
        // Use PID_BROWSER instead when sending a message to the browser process.
        context->GetBrowser()->SendProcessMessage(PID_BROWSER, msg);

        
        // Return value in JavaScript
        retval = CefV8Value::CreateString(message.c_str());
      }

      return true;
    }
    
    // Function does not exist.
    return false;
  }
  
  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(MyV8Handler);
};
  
void setupMyCustomJSAPI(CefRefPtr<CefV8Context> &context) {

  // Set up handler for att functions
  CefRefPtr<CefV8Handler> handler = new MyV8Handler();
  
  // Create V8 object for OSC API
  CefRefPtr<CefV8Value> containerObject = CefV8Value::CreateObject(NULL);

  // Attach to global V8 object
  context->GetGlobal()->SetValue("JuxtOSC", containerObject, V8_PROPERTY_ATTRIBUTE_NONE);
  
  // Set up send message function
  CefRefPtr<CefV8Value> oscSendMessage = CefV8Value::CreateFunction("send", handler);
  containerObject->SetValue("send", oscSendMessage, V8_PROPERTY_ATTRIBUTE_NONE);
}

void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextCreated(this, browser, frame, context);
  
  setupMyCustomJSAPI(context);
}

void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextReleased(this, browser, frame, context);
}

void ClientApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientApp::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefDOMNode> node) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnFocusedNodeChanged(this, browser, frame, node);
}

bool ClientApp::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  bool handled = false;

  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
  }

  return handled;
}

}  // namespace client
