// httplib is only included here — never in web_server.h — so the Arduino
// min/max macros (defined by arduino_compat.h in other TUs) can't interfere
// with httplib's use of <random> and other system headers that have zero-arg
// member functions named min()/max().

#define CPPHTTPLIB_THREAD_POOL_COUNT 1
#include <httplib.h>

#include "stubs/web_server.h"

#include <iostream>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
//  Implementation detail
// ---------------------------------------------------------------------------

struct WebServerImpl {
  httplib::Server svr;
};

// Thread-local request/response context, set for each handler call.
thread_local static const httplib::Request*                           t_req = nullptr;
thread_local static httplib::Response*                                t_res = nullptr;
thread_local static std::vector<std::pair<std::string,std::string>>  t_hdrs;

static void applyHeaders(httplib::Response& res) {
  for (auto& h : t_hdrs) res.set_header(h.first.c_str(), h.second.c_str());
  t_hdrs.clear();
}

static void dispatch(const httplib::Request& req, httplib::Response& res,
                     const THandlerFunction& handler) {
  t_req = &req;
  t_res = &res;
  t_hdrs.clear();

  std::cerr << req.method << " " << req.path;
  if (!req.params.empty()) {
    std::cerr << "?";
    bool first = true;
    for (auto& p : req.params) {
      if (!first) std::cerr << "&";
      std::cerr << p.first << "=" << p.second;
      first = false;
    }
  }
  if (!req.body.empty()) std::cerr << " body=" << req.body;
  std::cerr << "\n";

  handler();
  applyHeaders(res);

  std::cerr << "  -> " << res.status << "\n";

  t_req = nullptr;
  t_res = nullptr;
}

// ---------------------------------------------------------------------------
//  WebServerStub
// ---------------------------------------------------------------------------

WebServerStub::WebServerStub()  : impl_(std::make_unique<WebServerImpl>()) {}
WebServerStub::~WebServerStub() = default;

void WebServerStub::on(const char* uri, int method, THandlerFunction handler) {
  if (method == HTTP_GET || method == HTTP_ANY) {
    impl_->svr.Get(uri, [handler](const httplib::Request& req, httplib::Response& res) {
      dispatch(req, res, handler);
    });
  }
  if (method == HTTP_POST || method == HTTP_ANY) {
    impl_->svr.Post(uri, [handler](const httplib::Request& req, httplib::Response& res) {
      dispatch(req, res, handler);
    });
  }
}

void WebServerStub::on(const char* uri, int /*method*/,
                       THandlerFunction /*done*/, THandlerFunction /*stream*/) {
  impl_->svr.Post(uri, [](const httplib::Request&, httplib::Response& res) {
    res.status = 501;
    res.set_content("File upload not supported in the web emulator.", "text/plain");
  });
}

void WebServerStub::send(int code, const char* contentType, const String& body) {
  if (!t_res) return;
  t_res->status = code;
  applyHeaders(*t_res);
  t_res->set_content(body.c_str(), contentType);
}

void WebServerStub::send_P(int code, const char* contentType, const char* progmemStr) {
  if (!t_res) return;
  t_res->status = code;
  applyHeaders(*t_res);
  t_res->set_content(progmemStr, contentType);
}

void WebServerStub::sendHeader(const char* name, const String& value) {
  t_hdrs.push_back({name, value.c_str()});
}

String WebServerStub::arg(const String& name) {
  if (!t_req) return String("");
  if (t_req->has_param(name.c_str()))
    return String(t_req->get_param_value(name.c_str()).c_str());
  return String("");
}

bool WebServerStub::hasArg(const String& name) {
  return t_req && t_req->has_param(name.c_str());
}

void WebServerStub::run(int port) {
  std::cout << "Pala One web emulator: http://localhost:" << port << "\n";
  std::cout << "Press Ctrl+C to stop.\n";
  impl_->svr.listen("0.0.0.0", port);
}

void WebServerStub::stop() {
  impl_->svr.stop();
}
