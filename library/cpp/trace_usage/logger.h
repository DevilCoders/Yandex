#pragma once

#include "memory_registry.h"

#include <library/cpp/inf_buffer/inf_buffer.h>

#include <util/stream/output.h>
#include <util/system/spinlock.h>

namespace NTraceUsage {
    class TUsageLogger: public TMemoryRegistry {
    private:
        NInfBuffer::TInfBuffer Impl;

    public:
        TUsageLogger(const TString& fileName);
        TUsageLogger(THolder<IOutputStream> output);
        ~TUsageLogger();

        void ConsumeReport(TArrayRef<const ui8> report) noexcept final;
    };

}
