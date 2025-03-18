#pragma once

#include "config.h"

#include <util/stream/output.h>
#include <util/stream/trace.h>
#include <util/thread/pool.h>

namespace NRemorphCompiler {

class TCompiler {
private:
    TThreadPool Queue;
    size_t Threads;

public:
    TCompiler(size_t threads = 1, ETraceLevel verbosity = TRACE_WARN);

    bool Run(const TConfig& config, IOutputStream* log = nullptr, bool throwOnError = false);
};

} // NRemorphCompiler
