// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_REQUEST_CONTEXT_H_
#define SDCH_REQUEST_CONTEXT_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <memory>

#include "sdch_module.h"

#include "sdch_dictionary.h"
#include "sdch_fastdict_factory.h"

namespace sdch {

class Handler;

// Context used inside nginx to keep relevant data.
class RequestContext {
 public:
  // Create RequestContext.
  explicit RequestContext(ngx_http_request_t* r);

  // Fetch RequestContext associated with nginx request
  static RequestContext* get(ngx_http_request_t* r);

  ngx_http_request_t* request;
  Handler*            handler;

  bool need_flush;  // FIXME

  bool started : 1;
  bool done : 1;

  size_t total_in;
  size_t total_out;
};


}  // namespace sdch

#endif  // SDCH_REQUEST_CONTEXT_H_

