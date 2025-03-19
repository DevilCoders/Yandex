#include "processor.h"
#include <library/cpp/logger/global/global.h>
#include <kernel/daemon/messages.h>
#include <kernel/common_proxy/unistat_signals/signals.h>
#include <util/system/thread.h>

namespace NCommonProxy {

    const TMetaData& IStrictDataProcessor::GetInputMetaData() const {
        return InputMeta;
    }

    const TMetaData& IStrictDataProcessor::GetOutputMetaData() const {
        return OutputMeta;
    }

    TProcessor::TPtr TProcessor::Create(const TString& name, const TProcessorsConfigs& config) {
        const TProcessorConfig* cfg = config.Get<TProcessorConfig>(name);
        CHECK_WITH_LOG(cfg);
        return TFactory::Construct(cfg->GetType(), name, config);
    }

    TProcessor::TProcessor(const TString& name, const TProcessorsConfigs& configs)
        : ProcessedCounter("processed")
        , Config(*configs.Get<TProcessorConfig>(name))
        , Queue(this, TThreadPool::TParams().SetBlocking(true).SetCatching(true))
    {
        // thread name is limited by 15 symbols on some platforms
        ThreadsName = GetName();
        if (ThreadsName.length() > 15) {
            ThreadsName = ThreadsName.substr(0, 7) + "." + ThreadsName.substr(ThreadsName.length() - 7);
        }
        RegisterGlobalMessageProcessor(this);
    }


    void TProcessor::AddRequester(TLink::TPtr /*link*/) {
    }

    TProcessor::~TProcessor() {
        UnregisterGlobalMessageProcessor(this);
    }

    TString TProcessor::GetName() const {
        return Config.ComponentName;
    }

    void TProcessor::AddListener(TLink::TPtr link) {
        Links.emplace_back(link);
    }

    void TProcessor::ProcessImpl(TDataSet::TPtr input, IReplier::TPtr replier, TInstant enqueued) const {
        if (replier->Canceled()) {
            replier->AddReply(GetName(), 408, "Canceled");
            return;
        }
        TInstant start = Now();
        replier->AddTrace(GetName() + " start process");
        PushProcessorSignal(GetName(), "wait_time", (enqueued - start).MicroSeconds());
        PushProcessorSignal(GetName(), "input_age", (start - replier->GetStartTime()).MilliSeconds());
        try {
            const TMetaData& inputMD = GetInputMetaData();
            if (!inputMD.GetFields().empty() && (!input || !inputMD.IsSubsetOf(input->GetMetaData())))
                ythrow yexception() << "invalid input metadata: " << input->GetMetaData().ToString() << ", must be " << inputMD.ToString();
            DoProcess(input, replier);
            ProcessedCounter.Hit();
        } catch (...) {
            replier->AddReply(GetName(), 500, CurrentExceptionMessage());
        }
        PushProcessorSignal(GetName(), "processed", 1);
        ui64 time = (Now() - start).MicroSeconds();
        AtomicAdd(SpendTime, time / 1000);
        PushProcessorSignal(GetName(), "process_time", time);
        replier->AddTrace(GetName() + " end process");
    }

    TString TProcessor::Name() const {
        return GetName();
    }

    void TProcessor::Process(TDataSet::TPtr input, IReplier::TPtr replier) const {
        Started.Wait();
        if (replier->Canceled()) {
            replier->AddReply(GetName(), 408, "Canceled");
            return;
        }
        TInstant enqueued = Now();
        replier->AddTrace(GetName() + " add to queue");
        Queue.SafeAddFunc([this, input, replier, enqueued]() {ProcessImpl(input, replier, enqueued);});
    }

    void TProcessor::RegisterSignals(TUnistat& tass) const {
        if (!Config.IsIgnoredSignal("processed")) {
            tass.DrillFloatHole(TCommonProxySignals::GetSignalName(GetName(), "processed"), "dmmm", NUnistat::TPriority(50));
        }
        if (!Config.IsIgnoredSignal("queue_size")) {
            tass.DrillFloatHole(TCommonProxySignals::GetSignalName(GetName(), "queue_size"), "ammv", NUnistat::TPriority(50),
                NUnistat::TStartValue(0), EAggregationType::LastValue);
        }
        for (TString signalName : {"process_time", "input_age", "wait_time"}) {
            if (Config.IsIgnoredSignal(signalName)) {
                continue;
            }
            tass.DrillHistogramHole(TCommonProxySignals::GetSignalName(GetName(), signalName), "dhhh", NUnistat::TPriority(50),
                TCommonProxySignals::TimeIntervals);
        }
        for (TString codeStr : {"0xx", "1xx", "2xx", "3xx", "4xx", "5xx", "408"}) {
            if (Config.IsIgnoredSignal("poduce_error_" + codeStr)) {
                continue;
            }
            tass.DrillFloatHole(TCommonProxySignals::GetSignalName(GetName(), "poduce_error_" + codeStr), "dmmm", NUnistat::TPriority(50));
        }
        for (const auto& link : Links) {
            link->RegisterSignals(tass);
        }
        DoRegisterSignals(tass);
    }

    void* TProcessor::CreateThreadSpecificResource() const {
        TThread::SetCurrentThreadName(ThreadsName.c_str());
        return nullptr;
    }

    void TProcessor::DestroyThreadSpecificResource(void*) const {
    }

    void TProcessor::CollectInfo(NJson::TJsonValue& result) const {
        result["queue_size"] = Queue.Size();
        ProcessedCounter.WriteInfo(result);
        ui64 time = AtomicGet(SpendTime);
        result["work_time"] = time;
        if (ui64 processed = ProcessedCounter.GetCount())
            result["time_per_request"] = ((double)time) / processed;
    }


    void TProcessor::UpdateUnistatSignals() const {
        PushProcessorSignal(GetName(), "queue_size", Queue.Size());
    }

    bool TProcessor::Process(IMessage* message) {
        if (TCollectServerInfo* info = dynamic_cast<TCollectServerInfo*>(message)) {
            CollectInfo(info->Fields["processors"][GetName()]);
            return true;
        }
        if (dynamic_cast<TMessageUpdateUnistatSignals*>(message)) {
            UpdateUnistatSignals();
            return true;
        }
        return DoProcessMessage(message);
    }

    void TProcessor::Init() {

    }

    void TProcessor::Start() {
        Queue.Start(Config.GetThreads(), Config.GetMaxQueueSize());
        DoStart();
        Started.Signal();
    }

    void TProcessor::Run() {
    }

    void TProcessor::Stop() {
        DoStop();
    }

    void TProcessor::Wait() {
        DoWait();
        Queue.Stop();
    }

    void TProcessor::SendRequestToListeners(TDataSet::TPtr input, IReplier::TPtr replier) const {
        CHECK_WITH_LOG(Started.Wait(0));
        const TMetaData& outputMD = GetOutputMetaData();
        if (!outputMD.GetFields().empty() && (!input || !outputMD.IsSubsetOf(input->GetMetaData())))
            ythrow yexception() << "invalid output metadata: " << input->GetMetaData().ToString() << ", must be " << outputMD.ToString();
        for (const auto& link : Links) {
            link->ForwardRequest(input, replier);
        }
    }

    void TProcessor::PushProcessorSignal(const TString& processor, const TString& signalName, double value) const {
        if (!Config.IsIgnoredSignal(signalName)) {
            TCommonProxySignals::PushSpecialSignal(processor, signalName, value);
        }
    }
}
