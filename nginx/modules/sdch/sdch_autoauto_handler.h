// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_AUTOAUTO_HANDLER_H_
#define SDCH_AUTOAUTO_HANDLER_H_

#include <vector>

#include "sdch_handler.h"

namespace sdch {

class RequestContext;

// Create YaSDCH dictionary 
class AutoautoHandler : public Handler {
 public:
  AutoautoHandler(RequestContext* ctx, Handler* next);
  ~AutoautoHandler();

  virtual bool init(RequestContext* ctx);

  virtual ngx_int_t on_data(const uint8_t* buf, size_t len);
  virtual ngx_int_t on_finish();

 private:
  // Keep context. For logging purpose mostly.
  RequestContext* ctx_;

  // FastdictFactory for data passing by. We'll create actual dictionary in on_finish
  std::vector<char> blob_;
};


}  // namespace sdch

#endif  // SDCH_AUTOAUTO_HANDLER_H_

