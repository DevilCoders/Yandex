// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#include "sdch_dump_handler.h"

#include <cassert>
#include "sdch_config.h"
#include "sdch_request_context.h"

namespace sdch {

DumpHandler::DumpHandler(Handler* next) : Handler(next), fd_(-1) {
  assert(next_);
}

DumpHandler::~DumpHandler() {}

bool DumpHandler::init(RequestContext* ctx) {
  Config* conf = Config::get(ctx->request);
  char fn[conf->sdch_dumpdir.len + 30];

  sprintf(fn,
          "%s/%08lx-%08lx-%08lx",
          conf->sdch_dumpdir.data,
          random(),
          random(),
          random());

  fd_.reset(open(fn, O_WRONLY | O_CREAT | O_EXCL, 0666));
  if (fd_ == -1) {
    ngx_log_error(NGX_LOG_ERR,
                  ctx->request->connection->log,
                  0,
                  "dump open error %s",
                  fn);
    return false;
  }

  ngx_log_error(NGX_LOG_DEBUG,
                ctx->request->connection->log,
                0,
                "dump open file %s",
                fn);
  return true;
}

ngx_int_t DumpHandler::on_data(const uint8_t* buf, size_t len) {
  ssize_t res = 0;

  if ((res = write(fd_, buf, len)) != static_cast<ssize_t>(len)) {
    // XXX
    return NGX_ERROR;
  }

  if (next_)
    return next_->on_data(buf, len);
  return NGX_OK;
}

}  // namespace sdch
