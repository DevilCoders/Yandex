#include "neh.h"

#include <kernel/common_server/library/async_proxy/message.h>

#include <search/meta/scatter/source.h>
#include <search/session/logger/logger.h>

#include <library/cpp/http/io/stream.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/threading/future/future.h>

#include <library/cpp/string_utils/url/url.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace {
    NUtil::THttpReply GetReply(const NNeh::THttpAsyncReport& reply) {
        NUtil::THttpReply result;
        result.SetCode(reply.GetHttpCode());
        result.SetHeaders(reply.GetHeaders());
        if (reply.GetReport()) {
            result.SetContent(*reply.GetReport());
            if (result.Code() != 200) {
                result.AddFlag(NUtil::THttpReply::EFlags::ErrorInResponse);
            }
        } else {
            result.AddFlag(NUtil::THttpReply::EFlags::Timeout);
        }
        return result;
    }

    class TOwnCallback: public NNeh::THttpAsyncReport::ICallback {
    private:
        TAtomicSharedPtr<NNeh::THttpAsyncReport::ICallback> Callback;

    public:
        TOwnCallback(TAtomicSharedPtr<NNeh::THttpAsyncReport::ICallback> callback)
            : Callback(std::move(callback))
        {
        }

        void OnResponse(const TVector<NNeh::THttpAsyncReport>& reports) final {
            THolder<TOwnCallback> this_(this);
            if (!!Callback) {
                try {
                    Callback->OnResponse(reports);
                } catch (...) {
                    ERROR_LOG << CurrentExceptionMessage() << Endl;
                }
            }
        }
    };

    class TFutureCallback: public NNeh::THttpAsyncReport::ICallback {
    public:
        TFutureCallback(NThreading::TPromise<NUtil::THttpReply> promise)
            : Promise(promise)
        {
        }

        ~TFutureCallback() {
            if (!Promise.HasValue() && !Promise.HasException()) {
                Promise.SetException("callback terminated");
            }
        }

        void OnResponse(const TVector<NNeh::THttpAsyncReport>& reports) final {
            THolder<TFutureCallback> this_(this);
            if (reports.size() == 1) {
                Promise.SetValue(GetReply(reports[0]));
            } else {
                Promise.SetException("unexpected number of reports: " + ToString(reports.size()));
            }
        }

    private:
        NThreading::TPromise<NUtil::THttpReply> Promise;
    };
}

bool NNeh::THttpAsyncReport::ParseErrorReport(const TString& data, TString& result, const ui32 /*httpCode*/) {
    result = data;
    return true;
}

bool NNeh::THttpAsyncReport::ParseReport(const TString& data, const TString& firstLine, TString& result, ui32& httpCode) {
    if (firstLine) {
        try {
            httpCode = ParseHttpRetCode(firstLine);
        } catch (...) {
            ERROR_LOG << "ill-formed first line " << firstLine << " : " << CurrentExceptionMessage() << Endl;
            return false;
        }
    } else {
        httpCode = 200;
    }
    result = data;
    return true;
}

void NNeh::THttpAsyncReport::OnSystemError(const NNeh::TResponse* response) {
    if (!response) {
        return;
    }
    auto code = HTTP_INTERNAL_SERVER_ERROR;
    auto report = MakeAtomicShared<TString>(
        TStringBuilder() << "SystemError:" << response->GetSystemErrorCode() << ':' << response->GetErrorText()
    );
    SetReport(report, code, response->Headers, /*error=*/true);
}

void NNeh::THttpAsyncReport::THandlerCallback::OnResponse(const TVector<THttpAsyncReport>& reports) {
    Reports.resize(reports.size());
    for (ui32 i = 0; i < reports.size(); ++i) {
        Reports[i] = reports[i];
    }
    Handler.SafeAddAndOwn(THolder(this));
}

class TCustomMetaConfigOwner {
private:
    CSA_READONLY_MAYBE(NSimpleMeta::TConfig, OwnedConfig);
    const NSimpleMeta::TConfig* ExternalConfig;
public:
    TCustomMetaConfigOwner(const NSimpleMeta::TConfig* customMetaConfigOwned, const NSimpleMeta::TConfig& externalConfig) {
        if (customMetaConfigOwned) {
            OwnedConfig = *customMetaConfigOwned;
        }
        ExternalConfig = &externalConfig;
    }

    const NSimpleMeta::TConfig& GetConfig() const {
        return OwnedConfig ? *OwnedConfig : *ExternalConfig;
    }
};

class NNeh::THttpClient::THttpRequestShard : public TCustomMetaConfigOwner, public TShardISource {
private:
    NNeh::TMessage Message;
    TSourceData::TPtr Source;
    THttpRequest Request;
public:
    THttpRequestShard(IAsyncDelivery* asyncDelivery, TAsyncTask* owner, TSourceData::TPtr source, const NSimpleMeta::TConfig* customMetaConfig, const THttpRequest& request, const ui32 shardId)
        : TCustomMetaConfigOwner(customMetaConfig, source->GetConfig())
        , TShardISource(owner, source->GetSource(), TCustomMetaConfigOwner::GetConfig(), asyncDelivery, shardId)
        , Source(source)
        , Request(request)
    {
    }

    bool Prepare() override {
        Message = Source->MakeNehMessage(Request);
        return TShardISource::Prepare();
    }

    NNeh::TMessage BuildMessage(const ui32 /*att*/) const override {
        return Message;
    }
};

class NNeh::THttpClient::TShardsSimpleReportBuilder : public TShardsReportBuilder<THttpAsyncReport> {
private:
    const THttpStatusManagerConfig Config;
    THttpAsyncReport::ICallback* Callback = nullptr;
    mutable TEventLogger EventLogger;
    TSelfFlushLogFramePtr EventLogFrame;
    IEventLog* EventLog = nullptr;

protected:
    THttpAsyncReport BuildShardResult(IShardDelivery* shardInfo) override {
        return { Config, shardInfo };
    }

    void DoOnReplyReady() const override {
        if (Callback) {
            try {
                Callback->OnResponse(ShardsResults);
            } catch (...) {
                ERROR_LOG << CurrentExceptionMessage() << Endl;
            }
        }
    }

public:
    TShardsSimpleReportBuilder(const THttpStatusManagerConfig& config, THttpAsyncReport::ICallback* callback = nullptr, IEventLog* eventLog = nullptr)
        : Config(config)
        , Callback(callback)
        , EventLog(eventLog)
    {
        if (EventLog) {
            EventLogFrame = MakeIntrusive<TSelfFlushLogFrame>(*EventLog);
            EventLogFrame->ForceDump();
            EventLogger.AssignLog(EventLogFrame);
        }
    }

    const TVector<THttpAsyncReport>& GetReports() const {
        return ShardsResults;
    }

    IEventLogger* GetEventLogger() const override {
        if (EventLog) {
            return &EventLogger;
        }
        return nullptr;
    }
};

NNeh::THttpClient::TGuard::TGuard(TAtomicSharedPtr<TShardsSimpleReportBuilder> reportBuilder)
    : ReportBuilder(reportBuilder)
{
}

NNeh::THttpClient::TGuard::TGuard() {
}

NNeh::THttpClient::TGuard::~TGuard() {
}

IReportBuilder::TPtr NNeh::THttpClient::TGuard::GetReportBuilder() {
    return ReportBuilder;
}

const TVector<NNeh::THttpAsyncReport>& NNeh::THttpClient::TGuard::GetReports() {
    return ReportBuilder->GetReports();
}

NNeh::THttpClient::TGuard& NNeh::THttpClient::TGuard::Wait(const TInstant deadline /*= TInstant::Max()*/) {
    ReportBuilder->WaitReply(deadline);
    return *this;
}

TString NNeh::THttpClient::TSourceData::GetScriptWithReasks() const {
    const TString orginalScript = GetScript();
    TString result;
    for (ui32 i = 0; i < Config.GetMaxAttempts(); ++i) {
        result += orginalScript + " ";
    }
    return result;
}

NNeh::THttpClient::TSourceData::TSourceData(const TString& host, const ui16 port, const NSimpleMeta::TConfig& config, const TString& name, const bool isHttps /*= false*/, const TString& cert /*= ""*/, const TString& certKey /*= ""*/)
    : THttpRequestBuilder(host, port, isHttps, cert, certKey)
    , Config(config)
    , Source(NScatter::CreateSimpleSource(name, GetScriptWithReasks(), config.BuildSourceOptions()))
    , Name(name)
{
}

NNeh::THttpClient::TSourceData::~TSourceData() {
}

NNeh::THttpClient::THttpClient(TAsyncDelivery::TPtr delivery, IEventLog* eventLog /*= nullptr*/)
    : Delivery(delivery)
    , EventLog(eventLog)
{
}

NNeh::THttpClient::THttpClient(const NSimpleMeta::TConfig& config /*= NSimpleMeta::TConfig::ForRequester()*/, TAsyncDelivery::TPtr ad /*= nullptr*/)
    : AsyncDelivery(ad ? nullptr : MakeAtomicShared<TAsyncDelivery>())
    , Delivery(ad ? ad : AsyncDelivery)
    , EventLog(config.GetEventLog())
{
    if (AsyncDelivery) {
        AsyncDelivery->Start(config.GetThreadsStatusChecker(), config.GetThreadsSenders());
    }
}

NNeh::THttpClient::THttpClient(const TStringBuf url, const NSimpleMeta::TConfig& config /*= NSimpleMeta::TConfig::ForRequester()*/, TAsyncDelivery::TPtr ad /*= nullptr*/)
    : THttpClient(config, ad)
{
    try {
        RegisterSource(url, config);
    } catch (...) {
        if (AsyncDelivery) {
            AsyncDelivery->Stop();
        }
        throw;
    }
}

NNeh::THttpClient::~THttpClient() {
    if (AsyncDelivery) {
        AsyncDelivery->Stop();
    }
}

NNeh::THttpClient& NNeh::THttpClient::RegisterSource(const TString& name, const TString& host, const ui16 port, const NSimpleMeta::TConfig& config, const bool isHttps /*= false*/, const TString& cert /*= ""*/, const TString& certKey /*= ""*/) {
    auto source = MakeAtomicShared<TSourceData>(host, port, config, name, isHttps, cert, certKey);
    return RegisterSource(source);
}

NNeh::THttpClient& NNeh::THttpClient::RegisterSource(const TStringBuf url, const NSimpleMeta::TConfig& config, const TString& name) {
    TStringBuf scheme;
    TStringBuf host;
    ui16 port;
    Y_ENSURE(TryGetSchemeHostAndPort(url, scheme, host, port), "cannot parse " << url << " into scheme+host+port");

    auto h = ToString(host);
    bool https = scheme == "https://";
    const TString& source = name ? name : h;
    return RegisterSource(source, h, port, config, https);
}

NNeh::THttpClient& NNeh::THttpClient::RegisterSource(TSourceData::TPtr data) {
    TWriteGuard g(SourcesLock);
    CHECK_WITH_LOG(!Sources.contains(data->GetName()));
    Sources[data->GetName()] = data;
    return *this;
}

bool NNeh::THttpClient::HasSource(const TString& name) const {
    TReadGuard g(SourcesLock);
    return Sources.contains(name);
}

NNeh::THttpClient::TSourceData::TPtr NNeh::THttpClient::GetSource(const TString& name) const {
    TReadGuard g(SourcesLock);
    auto it = Sources.find(name);
    if (it != Sources.end()) {
        return it->second;
    }
    return nullptr;
}

NNeh::THttpClient::TSourceData::TPtr NNeh::THttpClient::GetUniqueSource() const {
    TReadGuard g(SourcesLock);
    CHECK_WITH_LOG(Sources.size() == 1) << Sources.size();
    return Sources.begin()->second;
}

NUtil::THttpReply NNeh::THttpClient::SendMessageSync(const THttpRequest& request, const TInstant cutoff) const {
    const TInstant deadline = cutoff ? cutoff : GetDeadline({}, request.GetConfigMeta());
    const THttpAsyncReport response = Send(request, deadline).Wait(deadline).GetReports().front();
    const NUtil::THttpReply& result = GetReply(response);
    return result;
}

NThreading::TFuture<NUtil::THttpReply> NNeh::THttpClient::SendAsync(const THttpRequest& request, const TInstant cutoff) const {
    auto deadline = cutoff ? cutoff : GetDeadline({}, request.GetConfigMeta());
    auto promise = NThreading::NewPromise<NUtil::THttpReply>();
    Send(request, deadline, new TFutureCallback(promise));
    return promise.GetFuture();
}

NThreading::TFuture<NUtil::THttpReply> NNeh::THttpClient::SendAsync(const TString& name, const THttpRequest& request, const TInstant cutoff) const {
    auto deadline = cutoff ? cutoff : GetDeadline(name, request.GetConfigMeta());
    auto promise = NThreading::NewPromise<NUtil::THttpReply>();
    auto requests = TMultiMap<TString, THttpRequest>({ {name, request} });
    Send(requests, deadline, new TFutureCallback(promise));
    return promise.GetFuture();
}

NNeh::THttpClient::TGuard NNeh::THttpClient::Send(const TMultiMap<TString, THttpRequest>& requests, const TInstant deadline, THolder<THttpAsyncReport::ICallback>&& callback) const {
    return Send(requests, deadline, new TOwnCallback(std::move(callback)));
}

NNeh::THttpClient::TGuard NNeh::THttpClient::Send(const THttpRequest& request, const TInstant deadline, THolder<THttpAsyncReport::ICallback>&& callback) const {
    TMultiMap<TString, THttpRequest> reqs;
    reqs.emplace(GetUniqueSource()->GetName(), request);
    return Send(reqs, deadline, std::move(callback));
}

NNeh::THttpClient::TGuard NNeh::THttpClient::SendPtr(const TMultiMap<TString, THttpRequest>& requests, const TInstant deadline, TAtomicSharedPtr<THttpAsyncReport::ICallback> callback) const {
    return Send(requests, deadline, new TOwnCallback(callback));
}

NNeh::THttpClient::TGuard NNeh::THttpClient::SendPtr(const THttpRequest& request, const TInstant deadline, TAtomicSharedPtr<THttpAsyncReport::ICallback> callback) const {
    TMultiMap<TString, THttpRequest> reqs;
    reqs.emplace(GetUniqueSource()->GetName(), request);
    return SendPtr(reqs, deadline, callback);
}

namespace {
    class TSendPackCallback: public NNeh::THttpAsyncReport::ICallback {
    private:
        TRWMutex& StopMutex;
        TAtomic& Counter;
        const ui32 Idx;
        TVector<NUtil::THttpReply>& Result;
        TReadGuard RGuard;
        TManualEvent& CounterEvent;
        const TInstant StartInstant = Now();
    public:
        TSendPackCallback(TManualEvent& counterEvent, TRWMutex& stopMutex, TAtomic& counter, TVector<NUtil::THttpReply>& result, const ui32 idx)
            : StopMutex(stopMutex)
            , Counter(counter)
            , Idx(idx)
            , Result(result)
            , RGuard(StopMutex)
            , CounterEvent(counterEvent)
        {
            AtomicIncrement(Counter);
        }

        ~TSendPackCallback() {
            AtomicDecrement(Counter);
            CounterEvent.Signal();
        }

        void OnResponse(const TVector<NNeh::THttpAsyncReport>& reports) override {
            if (reports.size() == 1) {
                Result[Idx].SetCode(reports.front().GetHttpCode());
                Result[Idx].SetStartInstant(StartInstant);
                if (!!reports.front().GetReport()) {
                    Result[Idx].SetContent(reports.front().GetReportSafe());
                    Result[Idx].SetHeaders(reports.front().GetHeaders());
                }
            }
        }
    };
}

TMap<TString, NUtil::THttpReply> NNeh::THttpClient::SendPack(const TMap<TString, THttpRequest>& requests,
                                                                const TInstant deadline,
                                                                const ui32 maxInFlight) const
{
    return SendPack(requests, deadline, maxInFlight, TDuration::Max());
}

TMap<TString, NUtil::THttpReply> NNeh::THttpClient::SendPack(const TMap<TString, THttpRequest>& requests,
                                                                const TInstant deadline,
                                                                const ui32 maxInFlight,
                                                                const TDuration reqTimeout) const
{
    TVector<THttpRequest> orderedRequests;
    for (auto&& reqInfo : requests) {
        orderedRequests.emplace_back(reqInfo.second);
    }
    auto preResult = SendPack(orderedRequests, deadline, maxInFlight, reqTimeout);
    ui64 idx = 0;
    TMap<TString, NUtil::THttpReply> result;
    for (auto&& reqInfo : requests) {
        result.emplace(reqInfo.first, std::move(preResult[idx]));
        ++idx;
    }
    return result;
}

TVector<NUtil::THttpReply> NNeh::THttpClient::SendPack(const TVector<THttpRequest>& requests,
                                                                const TInstant deadline,
                                                                const ui32 maxInFlight,
                                                                const TDuration reqTimeout) const
{
    {
        TReadGuard g(SourcesLock);
        CHECK_WITH_LOG(Sources.size() == 1) << Sources.size() << Endl;
    }
    TRWMutex stopMutex;
    TManualEvent eventWait;
    TAtomic counter = 0;
    TVector<NUtil::THttpReply> preResult(requests.size(), NUtil::THttpReply().SetCode(HTTP_INTERNAL_SERVER_ERROR));
    ui32 idx = 0;
    for (auto&& req : requests) {
        if (maxInFlight && AtomicGet(counter) > maxInFlight) {
            if (!eventWait.WaitD(deadline)) {
                break;
            }
            eventWait.Reset();
        }
        TInstant localDeadline = (reqTimeout == TDuration::Max()) ? deadline : Now() + reqTimeout;
        SendPtr(req, localDeadline, MakeAtomicShared<TSendPackCallback>(eventWait, stopMutex, counter, preResult, idx++));
    }

    TWriteGuard wg(stopMutex);
    return preResult;
}

NNeh::THttpClient::TGuard NNeh::THttpClient::Send(const THttpRequest& request, const TInstant deadline, THttpAsyncReport::ICallback* callback /*= nullptr*/) const {
    TMultiMap<TString, THttpRequest> reqs;
    reqs.emplace(GetUniqueSource()->GetName(), request);
    return Send(reqs, deadline, callback);
}

NNeh::THttpClient::TGuard NNeh::THttpClient::Send(const TMultiMap<TString, THttpRequest>& requests, const TInstant deadline, THttpAsyncReport::ICallback* callback /*= nullptr*/) const {
    ui32 idx = 0;
    TGuard result(new TShardsSimpleReportBuilder(THttpStatusManagerConfig(), callback, EventLog));
    auto task = MakeHolder<TAsyncTask>(result.GetReportBuilder(), deadline);
    for (auto&& i : requests) {
        auto sourcePtr = GetSource(i.first);
        CHECK_WITH_LOG(!!sourcePtr) << i.first;
        task->AddShard(MakeAtomicShared<THttpRequestShard>(Delivery.Get(), task.Get(), sourcePtr, i.second.GetConfigMeta(), i.second, idx++));
    }
    Delivery->Send(task.Release());
    return result;
}

TInstant NNeh::THttpClient::GetDeadline(const TString& source, const NSimpleMeta::TConfig* metaConfig) const {
    if (metaConfig) {
        return Now() + metaConfig->GetGlobalTimeout();
    } else {
        auto s = source ? GetSource(source) : GetUniqueSource();
        Y_ENSURE(s, "cannot find source " << source);
        return Now() + s->GetConfig().GetGlobalTimeout();
    }
}
