#include "logger.h"

#include <util/stream/file.h>
#include <util/stream/holder.h>
#include <util/stream/zlib.h>

namespace NTraceUsage {
    TUsageLogger::TUsageLogger(const TString& fileName)
        : Impl(
              MakeHolder<THoldingStream<TZLibCompress>>(
                  MakeHolder<TFileOutput>(fileName))) {
    }

    TUsageLogger::TUsageLogger(THolder<IOutputStream> output)
        : Impl(std::move(output))
    {
    }

    TUsageLogger::~TUsageLogger() {
    }

    void TUsageLogger::ConsumeReport(TArrayRef<const ui8> report) noexcept {
        ui32 reportSize = report.size();
        IOutputStream::TPart parts[2] = {
            IOutputStream::TPart(&reportSize, sizeof(reportSize)),
            IOutputStream::TPart(report.data(), reportSize)};
        Impl.Write(parts, 2);
    }

}
