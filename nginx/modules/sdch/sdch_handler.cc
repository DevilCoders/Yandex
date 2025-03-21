// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_handler.h"

namespace sdch {

Handler::Handler(Handler* next) : next_(next) {}

Handler::~Handler() {}

ngx_int_t Handler::on_finish() {
  if (next_)
    return next_->on_finish();
  return NGX_OK;
}

}  // namespace sdch
