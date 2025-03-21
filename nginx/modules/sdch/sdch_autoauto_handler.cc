// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_autoauto_handler.h"

#include "sdch_dictionary.h"
#include "sdch_main_config.h"
#include "sdch_request_context.h"

namespace sdch {

AutoautoHandler::AutoautoHandler(RequestContext* ctx, Handler* next)
    : Handler(next), ctx_(ctx) {
  // TODO
}

AutoautoHandler::~AutoautoHandler() {}

bool AutoautoHandler::init(RequestContext* ctx) {
  return true;
}

ngx_int_t AutoautoHandler::on_data(const uint8_t* buf, size_t len) {
  blob_.insert(blob_.end(), buf, buf + len);

  if (next_)
    return next_->on_data(buf, len);
  return NGX_OK;
}

ngx_int_t AutoautoHandler::on_finish() {
  if (blob_.empty()) {
    ngx_log_error(NGX_LOG_ERR,
                  ctx_->request->connection->log,
                  0,
                  "storing quasidict: no blob");
  } else {
    MainConfig* main = MainConfig::get(ctx_->request);
    Dictionary* dict =
        main->fastdict_factory.create_dictionary(blob_.data(), blob_.size());

    if (dict) {
      Dictionary::id_t client_id = dict->client_id();
      ngx_log_error(NGX_LOG_DEBUG,
                    ctx_->request->connection->log,
                    0,
                    "storing quasidict %*s (%d)",
                    client_id.size(), client_id.data(),
                    blob_.size());
    } else {
      ngx_log_error(NGX_LOG_ERR,
                    ctx_->request->connection->log,
                    0,
                    "failed storing quasidict (%d)",
                    blob_.size());
    }
  }

  return next_->on_finish();
}

}  // namespace sdch
