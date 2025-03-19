#include "flushing_stream_backend.h"

#include <library/cpp/logger/record.h>

#include <util/stream/output.h>

NOxygen::TFlushingStreamLogBackend::TFlushingStreamLogBackend(IOutputStream* slave)
    : Slave(slave)
{
}

NOxygen::TFlushingStreamLogBackend::~TFlushingStreamLogBackend() {
}

void NOxygen::TFlushingStreamLogBackend::WriteData(const TLogRecord& rec) {
    Slave->Write(rec.Data, rec.Len);
    Slave->Flush();
}

void NOxygen::TFlushingStreamLogBackend::ReopenLog() {
}
