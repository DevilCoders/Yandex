// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_main_config.h"

#include "sdch_module.h"

namespace sdch {

MainConfig::MainConfig() : stor_size(NGX_CONF_UNSET_SIZE) {}

MainConfig::~MainConfig() {}

MainConfig* MainConfig::get(ngx_http_request_t* r) {
  return static_cast<MainConfig*>(ngx_http_get_module_main_conf(r, sdch_module));
}

}  // namespace sdch
