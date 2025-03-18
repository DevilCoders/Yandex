#pragma once

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/stream/input.h>

namespace NTraceUsage {
    class TEventReportProto;

    class TLogReader: public NNonCopyable::TNonCopyable {
    private:
        TBuffer Buffer;
        THolder<IInputStream> Input;

    public:
        TLogReader(const TString& fileName);
        TLogReader(THolder<IInputStream> input);

        bool Next(TEventReportProto& eventReport);
    };

}
