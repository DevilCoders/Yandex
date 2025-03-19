#pragma once

#include <library/cpp/logger/backend.h>

class IOutputStream;

namespace NOxygen {

    class TFlushingStreamLogBackend : public TLogBackend {
    private:
        IOutputStream* Slave;

    public:
        TFlushingStreamLogBackend(IOutputStream* slave);
        ~TFlushingStreamLogBackend() override;

        void WriteData(const TLogRecord& rec) override;
        void ReopenLog() override;
    };

} // namespace NOxygen
