#pragma once

#include <library/cpp/logger/global/global.h>
#include <util/generic/yexception.h>

#define TRY try {
#define CATCH3(msg, rethrow, StreamLog)                                                \
  } catch (...) {                                              \
    StreamLog << "Error on " << msg << ": " << CurrentExceptionMessage() << Endl;   \
    if (rethrow) throw;                                                   \
  }

#define CATCH(msg) CATCH3(msg, false, ERROR_LOG)
#define CATCH_AND_RETHROW(msg) CATCH3(msg, true, ERROR_LOG)
