#include "abstract.h"

#include <kernel/common_server/library/json/builder.h>
#include <kernel/common_server/library/json/cast.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/library/unistat/signals.h>

IFrontendReportBuilder::IFrontendReportBuilder(IReplyContext::TPtr context, const TString& processor)
    : HandlerName(processor)
    , Context(context)
{
    if (!!Context) {
        const TCgiParameters& cgi = Context->GetCgiParameters();
        if (cgi.Has("eventlog_level")) {
            TryFromString(cgi.Get("eventlog_level"), MutableEventLogLevel());
        } else if (cgi.Has("dump", "eventlog")) {
            MutableEventLogLevel() = ELogPriority::TLOG_INFO;
        } else {
            MutableEventLogLevel() = ELogPriority::TLOG_EMERG;
            SetAccumulatorEnabled(false);
        }
        if (cgi.Has("eventlog_events_limit")) {
            TryFromString(cgi.Get("eventlog_events_limit"), MutableRecordsLimit());
        }
    }
}

IFrontendReportBuilder::IFrontendReportBuilder(const TCtx& ctx)
    : IFrontendReportBuilder(ctx.Context, ctx.Handler)
{
    AccessControlAllowOrigin = ctx.AccessControlAllowOrigin;
}

IFrontendReportBuilder::~IFrontendReportBuilder() {
    if (!AtomicGet(FinishFlag)) {
        TCSSignals::Signal("http_handlers")("handler", GetHandlerName())("metric", "reply.count")("http_code", "incorrect_finish")(Tags);
        SendGlobalMessage<TIncorrectFinishRequestMessage>();
    }
}

void IFrontendReportBuilder::DoAddRecord(const NCS::NLogging::TBaseLogRecord& r) {
    auto g = MakeThreadSafeGuard();
    DoAddEvent(Now(), r);
}

void IFrontendReportBuilder::Finish(const TCodedException& e) {
    if (AtomicCas(&FinishFlag, 1, 0)) {
        auto deadlineCheckResult = Context->DeadlineCheck(1);
        if (deadlineCheckResult == EDeadlineCorrectionResult::dcrRequestExpired) {
            FinishedCode = HTTP_GATEWAY_TIME_OUT;
            TCSSignals::Signal("http_handlers")("handler", GetHandlerName())("metric", "expired_on_finish")("http_code", FinishedCode)(Tags);
        } else {
            FinishedCode = e.GetCode();
            TCSSignals::Signal("http_handlers")("handler", GetHandlerName())("metric", "reply.count")("http_code", FinishedCode)(Tags);
        }
        DoFinish(e);
        TCSSignals::HSignal("http_handlers_duration", NRTProcHistogramSignals::IntervalsRTLineReply)
                ("handler", GetHandlerName())
                ("metric", "reply.times")
                ("http_code", FinishedCode)
                (Tags)
                ((Now() - Context->GetRequestStartTime()).MilliSeconds());
    }
}

IFrontendReportBuilder::TGuard::TGuard(IFrontendReportBuilder::TPtr report, const int code)
    : Report(report)
    , ErrorsInfo(code)
{
    EvLogGuard = TFLEventLog::ReqEventLogGuard("full_request");
}

IFrontendReportBuilder::TGuard::~TGuard() {
    EvLogGuard = nullptr;
    Flush();
}

IFrontendReportBuilder::TPtr IFrontendReportBuilder::TGuard::Release() {
    IFrontendReportBuilder::TPtr report;
    if (AtomicCas(&Flushed, 1, 0)) {
        report = std::move(Report);
    }
    Y_ASSERT(!Report);
    return report;
}

void IFrontendReportBuilder::TGuard::Flush() {
    if (AtomicCas(&Flushed, 1, 0) && Report) {
        Report->Finish(ErrorsInfo);
        Report.Drop();
    }
}
