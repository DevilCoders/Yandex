#pragma once

#include <library/cpp/logger/backend.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

class TUnistatLogBackend: public TLogBackend {
public:
    TUnistatLogBackend() = default;
    TUnistatLogBackend(const TString& name, ELogPriority priority = LOG_DEF_PRIORITY, bool threaded = false);
    TUnistatLogBackend(THolder<TLogBackend>&& backend);

    virtual void WriteData(const TLogRecord& rec) override;
    virtual void ReopenLog() override;

private:
    THolder<TLogBackend> Backend;
};
