#include "logreader.h"

#include <library/cpp/trace_usage/protos/event.pb.h>

#include <util/stream/file.h>
#include <util/stream/holder.h>
#include <util/stream/zlib.h>

namespace NTraceUsage {
    TLogReader::TLogReader(const TString& fileName)
        : Input(
              new THoldingStream<TBufferedZLibDecompress>(
                  MakeHolder<TIFStream>(fileName))) {
    }
    TLogReader::TLogReader(THolder<IInputStream> input)
        : Input(std::move(input))
    {
    }

    bool TLogReader::Next(TEventReportProto& eventReport) {
        ui32 length = 0;
        if (Input->Load(&length, sizeof(length)) != sizeof(length)) {
            return false;
        }
        Buffer.Resize(length);
        if (Input->Load(Buffer.data(), length) != length) {
            return false;
        }
        eventReport.Clear();
        if (!eventReport.ParseFromArray(Buffer.data(), length)) {
            return false;
        }
        return true;
    }

}
