// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_DUMP_HANDLER_H_
#define SDCH_DUMP_HANDLER_H_

#include "sdch_fdholder.h"
#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// Dumps data into temporary directory
class DumpHandler : public Handler {
 public:
  explicit DumpHandler(Handler* next);
  ~DumpHandler();

  virtual bool init(RequestContext* ctx);

  virtual ngx_int_t on_data(const uint8_t* buf, size_t len);

 private:
  FDHolder fd_;
};


}  // namespace sdch

#endif  // SDCH_DUMP_HANDLER_H_

