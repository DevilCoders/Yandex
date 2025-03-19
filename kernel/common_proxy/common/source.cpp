#include "source.h"

namespace NCommonProxy {

    void TSource::TConfig::DoInit(const TYandexConfig::Section& section) {
        TProcessorConfig::DoInit(section);
        const auto& dir = section.GetDirectives();
        dir.GetValue("MaxInProcess", MaxInProcess);
        dir.GetValue("UseSemaphoreForLimit", UseSemaphoreForLimit);
    }

    void TSource::TConfig::DoToString(IOutputStream& so) const {
        TProcessorConfig::DoToString(so);
        so << "MaxInProcess: " << MaxInProcess << Endl;
        so << "UseSemaphoreForLimit: " << UseSemaphoreForLimit << Endl;
    }

    TSource::TSource(const TString& name, const TProcessorsConfigs& configs)
        : TProcessor(name, configs)
        , FailedCounter("failed")
        , Config(*configs.Get<TConfig>(name))
        , MaxInProcess((Config.UseSemaphoreForLimit && Config.MaxInProcess > 0) ? MakeHolder<TFastSemaphore>(Config.MaxInProcess) : nullptr)
    {}

    const NCommonProxy::TMetaData& TSource::GetInputMetaData() const  {
        return GetOutputMetaData();
    }

    void TSource::DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const {
        SendRequestToListeners(input, replier);
    }

    void TSource::CollectInfo(NJson::TJsonValue& result) const {
        TProcessor::CollectInfo(result);
        FailedCounter.WriteInfo(result);
        result["in_process"] = AtomicGet(InProcess);
    }

    void TSource::UpdateUnistatSignals() const {
        TProcessor::UpdateUnistatSignals();
        TCommonProxySignals::PushSpecialSignal(GetName(), "in_process", AtomicGet(InProcess));
    }

    TSource::TReplier::TReplier(const TSource& owner, TReporter* reporter)
        : Source(owner)
        , Reporter(reporter ? reporter : new TReporter(*this))
    {
        if (Source.Config.UseSemaphoreForLimit) {
            if (Source.MaxInProcess) {
                Source.MaxInProcess->Acquire();
            }
        } else {
            while (Source.Config.MaxInProcess && AtomicGet(Source.InProcess) >= Source.Config.MaxInProcess) {
                Sleep(TDuration::MilliSeconds(10));
            }
        }
        AtomicIncrement(Source.InProcess);
    }

    void TSource::TReplier::AddReply(const TString& processorName, int code /*= 200*/, const TString& message /*= Default<TString>()*/, TDataSet::TPtr data /*= nullptr*/) {
        TGuard<TMutex> g(Mutex);
        if (code >= CommonCode && (code != 200 || !message.empty())) {
            CommonCode = code;
            NJson::TJsonValue& val = Message[processorName];
            val["code"] = code;
            val["message"] = message;
            TCommonProxySignals::PushSpecialSignal(processorName, "poduce_error_" + CodeToSignal(code), 1);
        }
        DoAddReply(processorName, code, message, data);
    }

    void TSource::TReplier::AddMessage(const TString& processorName, int code, const NJson::TJsonValue& msg) {
        TGuard<TMutex> g(Mutex);
        Message[processorName] = msg;
        CommonCode = Max<int>(CommonCode, code);
    }

    TSource::TReplier::~TReplier() {
        try {
            AtomicDecrement(Source.InProcess);
            if (Source.Config.UseSemaphoreForLimit) {
                if (Source.MaxInProcess) {
                    Source.MaxInProcess->Release();
                }
            }
            if (CommonCode == 200) {
                for (const auto& p : Message.GetMap()) {
                    const NJson::TJsonValue& msg = p.second["message"];
                    if (msg.GetString()) {
                        Reporter->Report(msg.GetString());
                        SendSignals();
                        return;
                    }
                }
            }
            Reporter->Report(Message.GetStringRobust());
            Reporter.Reset();
            SendSignals();
        } catch (...) {
            ERROR_LOG << "cannot send reply: " << CurrentExceptionMessage() << Endl;
        }
    }

    void TSource::TReplier::DoAddReply(const TString& /*processorName*/, int /*code*/, const TString& /*message*/, TDataSet::TPtr /*data*/) {
    }


    void TSource::TReplier::SendSignals() const {
        const TString codeStr = CodeToSignal(CommonCode);
        ui64 duration = (Now() - GetStartTime()).MilliSeconds();
        TCommonProxySignals::PushSpecialSignal(Source.GetName(), "reply", 1);
        TCommonProxySignals::PushSpecialSignal(Source.GetName(), "reply_" + codeStr, 1);
        TCommonProxySignals::PushSpecialSignal(Source.GetName(), "reply_time_" + codeStr, duration);
        TCommonProxySignals::PushSpecialSignal(Source.GetName(), "reply_time", duration);
    }

    TString TSource::TReplier::CodeToSignal(int code) {
        switch (code) {
        case  408:
            return "408";
        default:
            return ToString(Min(code / 100, 5)) + "xx";
        }
    }

    void TSource::DoRegisterSignals(TUnistat& tass) const {
        TProcessor::DoRegisterSignals(tass);
        tass.DrillFloatHole(TCommonProxySignals::GetSignalName(GetName(), "in_process"), "ammv", NUnistat::TPriority(50), NUnistat::TStartValue(0), EAggregationType::LastValue);
        tass.DrillHistogramHole(TCommonProxySignals::GetSignalName(GetName(), "reply_time"), "dhhh", NUnistat::TPriority(50),
            TCommonProxySignals::TimeIntervals);
        tass.DrillFloatHole(TCommonProxySignals::GetSignalName(GetName(), "reply"), "dmmm", NUnistat::TPriority(50));
        for (TString codeStr : {"0xx", "1xx", "2xx", "3xx", "4xx", "5xx", "408"}) {
            tass.DrillFloatHole(TCommonProxySignals::GetSignalName(GetName(), "reply_" + codeStr), "dmmm", NUnistat::TPriority(50));
            tass.DrillHistogramHole(TCommonProxySignals::GetSignalName(GetName(), "reply_time_" + codeStr), "dhhh", NUnistat::TPriority(50),
                TCommonProxySignals::TimeIntervals);
        }
    }

    void TSource::DoStart() {
    }

    TSource::TReplier::TReporter::TReporter(TReplier& owner)
        : Owner(owner)
    {}

    void TSource::TReplier::TReporter::Report(const TString& message) {
        ui64 duration = (Now() - Owner.GetStartTime()).MilliSeconds();
        if (Owner.CommonCode > 399) {
            ERROR_LOG << "source=" << Owner.Source.GetName() << ";code=" << Owner.CommonCode << "; message=" << message << "; duration=" << duration << Endl;
            Owner.Source.FailedCounter.Hit();
        } else {
            DEBUG_LOG << "source=" << Owner.Source.GetName() << ";code=" << Owner.CommonCode << "; duration=" << duration << Endl;
        }
    }

}
