// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_MAIN_CONFIG_H_
#define SDCH_MAIN_CONFIG_H_

extern "C" {
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_config.h>
}

#include "sdch_fastdict_factory.h"

namespace sdch {

class MainConfig {
 public:
  MainConfig();
  ~MainConfig();

  static MainConfig* get(ngx_http_request_t* r);

  FastdictFactory fastdict_factory;
  // TODO Change config handling to pass it to FastdictFactory directly
  ngx_uint_t stor_size;
};


}  // namespace sdch

#endif  // SDCH_MAIN_CONFIG_H_

