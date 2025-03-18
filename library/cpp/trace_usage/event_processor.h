#pragma once

#include <util/generic/ptr.h>

namespace NTraceUsage {
    class TEventReportProto;

    class IEventProcessor: public TThrRefBase, private NNonCopyable::TNonCopyable {
    public:
        virtual void ProcessEvent(const TEventReportProto& eventReport) = 0;
    };

    using TEventProcessorPtr = TIntrusivePtr<IEventProcessor>;

}
