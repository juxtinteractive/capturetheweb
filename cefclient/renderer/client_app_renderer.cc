// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/renderer/client_app_renderer.h"

#include "include/base/cef_logging.h"

namespace client {

ClientAppRenderer::ClientAppRenderer() {
}

void ClientAppRenderer::OnRenderThreadCreated(
    CefRefPtr<CefListValue> extra_info) {
  CreateDelegates(delegates_);

  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnRenderThreadCreated(this, extra_info);
}

void ClientAppRenderer::OnWebKitInitialized() {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnWebKitInitialized(this);
}

void ClientAppRenderer::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnBrowserCreated(this, browser);
}

void ClientAppRenderer::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnBrowserDestroyed(this, browser);
}

CefRefPtr<CefLoadHandler> ClientAppRenderer::GetLoadHandler() {
  CefRefPtr<CefLoadHandler> load_handler;
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !load_handler.get(); ++it)
    load_handler = (*it)->GetLoadHandler(this);

  return load_handler;
}

bool ClientAppRenderer::OnBeforeNavigation(CefRefPtr<CefBrowser> browser,
                                           CefRefPtr<CefFrame> frame,
                                           CefRefPtr<CefRequest> request,
                                           NavigationType navigation_type,
                                           bool is_redirect) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
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


void ClientAppRenderer::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefV8Context> context) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnContextCreated(this, browser, frame, context);

  setupMyCustomJSAPI(context);
}

void ClientAppRenderer::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefRefPtr<CefV8Context> context) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnContextReleased(this, browser, frame, context);
}

void ClientAppRenderer::OnUncaughtException(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context,
    CefRefPtr<CefV8Exception> exception,
    CefRefPtr<CefV8StackTrace> stackTrace) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientAppRenderer::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                             CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefDOMNode> node) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it)
    (*it)->OnFocusedNodeChanged(this, browser, frame, node);
}

bool ClientAppRenderer::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  bool handled = false;

  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
  }

  return handled;
}

}  // namespace client
