#include "backend.h"

#include <kernel/common_server/library/unistat/signals.h>

#include <library/cpp/logger/log.h>

#include <array>

TUnistatLogBackend::TUnistatLogBackend(const TString& name, ELogPriority priority /*= LOG_DEF_PRIORITY*/, bool threaded /*= false*/)
    : Backend(CreateLogBackend(name, priority, threaded))
{
}

TUnistatLogBackend::TUnistatLogBackend(THolder<TLogBackend>&& backend)
    : Backend(std::move(backend))
{
}

void TUnistatLogBackend::WriteData(const TLogRecord& rec) {
    TCSSignals::Signal("global_log")("priority", ::ToString(rec.Priority));
    if (Backend) {
        Backend->WriteData(rec);
    }
}

void TUnistatLogBackend::ReopenLog() {
    if (Backend) {
        Backend->ReopenLog();
    }
}
