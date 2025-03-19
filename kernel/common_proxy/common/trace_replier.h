#pragma once

#include "replier_decorator.h"

#include <util/generic/deque.h>
#include <util/system/mutex.h>
#include <util/datetime/base.h>

namespace NCommonProxy {

    class TTraceReplier : public TReplierDecorator {
    public:
        using TReplierDecorator::TReplierDecorator;
        virtual void AddTrace(const TString& comment) override;
        virtual void AddReply(const TString& processorName, int code = 200, const TString& message = Default<TString>(), TDataSet::TPtr data = nullptr) override;
        ~TTraceReplier() override;
    private:
        struct TFrame {
            TFrame(const TString& comment);
            TString ToString(TInstant startTime) const;
            TInstant Time;
            TString Comment;
        };
        TDeque<TFrame> Trace;
        TMutex Mutex;
    };
}
