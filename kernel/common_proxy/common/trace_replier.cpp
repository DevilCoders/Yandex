#include "trace_replier.h"
#include <util/string/builder.h>

namespace NCommonProxy {


    void TTraceReplier::AddTrace(const TString& comment) {
        TFrame f(comment);
        TGuard<TMutex> g(Mutex);
        Trace.emplace_back(f);
    }


    void TTraceReplier::AddReply(const TString& processorName, int code /*= 200*/, const TString& message /*= Default<TString>()*/, TDataSet::TPtr data /*= nullptr*/) {
        AddTrace(processorName + " add reply with code " + ::ToString(code));
        TReplierDecorator::AddReply(processorName, code, message, data);
    }


    TTraceReplier::~TTraceReplier() {
        NJson::TJsonValue msg(NJson::JSON_ARRAY);
        TInstant startTime = GetStartTime();
        for (const auto& f : Trace) {
            msg.AppendValue(f.ToString(startTime));
        }
        Slave->AddMessage("Trace", 999, msg);
    }


    TTraceReplier::TFrame::TFrame(const TString& comment)
        : Time(Now())
        , Comment(comment)
    {}


    TString TTraceReplier::TFrame::ToString(TInstant startTime) const {
        TStringBuilder ss;
        ss << Time.ToStringLocal() << " (+" << (Time - startTime).MicroSeconds() << " mks) " << Comment;
        return ss;
    }

}
