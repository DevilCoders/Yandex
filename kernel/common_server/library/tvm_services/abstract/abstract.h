#pragma once
#include "customizer/abstract.h"
#include "request/abstract.h"
#include "request/json.h"
#include <kernel/common_server/util/network/simple.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/obfuscator/obfuscators/abstract.h>

namespace NExternalAPI {

    class TSenderConfig: public TSimpleAsyncRequestSender::TConfig {
    private:
        using TBase = TSimpleAsyncRequestSender::TConfig;
        using THeaders = TMap<TString, TString>;
        CSA_READONLY_DEF(TString, UriPrefix);
        CSA_READONLY_DEF(TString, NecessaryUriSuffix);
        CSA_READONLY_DEF(THeaders, Headers);
        CSA_READONLY_DEF(TSet<TString>, HiddenCgiParameters);
        CSA_READONLY_DEF(TSet<TString>, LogHeadersReply);
        CSA_READONLY_DEF(TSet<TString>, LogHeadersRequest);
        CSA_READONLY_DEF(TString, AdditionalCgi);
        CSA_READONLY(ui32, MaxInFly, 10);
        CSA_READONLY(bool, LogEventsGlobal, true);
        CSA_READONLY(bool, LogEventsResponse, true);
        CSA_MUTABLE_DEF(TSenderConfig, TRequestCustomizerContainer, Customizer);
        CSA_DEFAULT(TSenderConfig, TString, HttpUserAgent);
        CS_ACCESS(TSenderConfig, ui32, ResponseSizeLimit, 1024 * 256);
        CS_ACCESS(TSenderConfig, ui32, RequestSizeLimit, 1024 * 16);
    public:
        void Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy);
        void ToString(IOutputStream& os) const;
    };

    class TSender {
    private:
        using THeaders = TMap<TString, TString>;
        const TSenderConfig Config;
        TSimpleAsyncRequestSender Impl;
        const TString ApiName;
        const IRequestCustomizationContext* CustomizationContext = nullptr;
        const NCS::NObfuscator::TObfuscatorManagerContainer ObfuscatorManager;

        CSA_MUTABLE_DEF(TSender, TMaybe<NUtil::THttpReply>, UniversalReply);

    protected:
        virtual bool DoTuneRequest(NNeh::THttpRequest& /*request*/) const {
            return true;
        }
    public:

        class TLinkGuard {
        public:
            TLinkGuard(const TString& trace);

            static TString GetTraceLink();

            ~TLinkGuard();
        };

        static TAtomicSharedPtr<TLinkGuard> InitLinks(const TString& traceLink) {
            return MakeAtomicShared<TLinkGuard>(traceLink);
        }

        const TString& GetApiName() const {
            return ApiName;
        }

        TCSSignals::TSignalBuilder Signal() const {
            return std::move(TCSSignals::Signal("sender_api")("sender", ApiName));
        }

        TCSSignals::TSignalBuilder SignalProblem() const {
            return std::move(TCSSignals::SignalProblem("sender_api")("sender", ApiName));
        }

        using TPtr = TAtomicSharedPtr<TSender>;

        TSender(const TSenderConfig& config, const TString& apiName, const IRequestCustomizationContext* cContext = nullptr)
            : Config(config)
            , Impl(Config)
            , ApiName(apiName)
            , CustomizationContext(cContext)
            , ObfuscatorManager(cContext ? cContext->GetObfuscatorManager() : NCS::NObfuscator::TObfuscatorManagerContainer())
        {
            if (!!Config.GetCustomizer()) {
                CHECK_WITH_LOG(Config.MutableCustomizer()->Start(*this)) << "cannot start customizer for " << apiName << Endl;
            }
        }

        const TSenderConfig& GetConfig() const {
            return Config;
        }

        virtual ~TSender() {
            if (!!Config.GetCustomizer()) {
                CHECK_WITH_LOG(Config.MutableCustomizer()->Stop()) << "cannot stop customizer for " << ApiName << Endl;
            }
        }

        template <class TRequest, class... TArgs>
        typename TRequest::TResponse SendRequest(TArgs... args) const {
            TRequest r(std::forward<TArgs>(args)...);
            return SendRequest(r);
        }

        template <class TRequest>
        TMap<TString, typename TRequest::TResponse> SendPack(const TMap<TString, TRequest>& requests) const {
            TMap<TString, typename TRequest::TResponse> result;
            TMap<TString, NNeh::THttpRequest> rawRequests;
            for (auto&&[id, req] : requests) {
                NNeh::THttpRequest request;
                bool isCorrectRequest = false;
                try {
                    isCorrectRequest = req.BuildHttpRequest(request);
                    if (isCorrectRequest) {
                        isCorrectRequest = TuneRequest(req, request);
                    }
                } catch (...) {
                    ERROR_LOG << CurrentExceptionMessage();
                }
                if (!isCorrectRequest) {
                    NUtil::THttpReply report;
                    report.SetErrorMessage("cannot_build_request");
                    if (!!request.GetUri()) {
                        SignalProblem()("uri", request.GetUri())("code", "cannot_build_request")("req_type", request.GetRequestType());
                    } else {
                        SignalProblem()("uri", "unknown")("code", "cannot_build_request")("req_type", request.GetRequestType());
                    }
                    LogError(report, request, req, TDuration::Zero());

                    typename TRequest::TResponse response;
                    response.SetCode(HTTP_BAD_REQUEST);
                    result[id] = response;
                    continue;
                }
                LogRequest(request, req);
                rawRequests[id] = request;
            }

            TDuration summaryTimeout = Config.GetRequestConfig().GetGlobalTimeout() * requests.size();
            const TInstant start = Now();
            auto rawReplies = Impl->SendPack(rawRequests, Now() + summaryTimeout, Config.GetMaxInFly(), Config.GetRequestConfig().GetGlobalTimeout());
            for (auto&&[id, rawReply] : rawReplies) {
                auto it = requests.find(id);
                auto itRaw = rawRequests.find(id);
                CHECK_WITH_LOG(it != requests.end() && itRaw != rawRequests.end());
                result[id] = ProcessReport(it->second, itRaw->second, rawReply, 0, rawReply.GetStartInstantMaybe().GetOrElse(start));
            }
            return result;
        }

        virtual bool TuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request) const noexcept final {
            auto gLogging = TFLRecords::StartContext().Method("TuneRequest");
            try {
                if (!!Config.GetCustomizer()) {
                    TFLEventLog::Log("customization_apply");
                    if (!Config.GetCustomizer()->TuneRequest(baseRequest, request, CustomizationContext, this)) {
                        return false;
                    }
                }
                for (auto&& i : Config.GetHeaders()) {
                    TFLEventLog::Log("add_post_customization_header")("header", i.first);
                    request.AddHeader(i.first, i.second);
                }
                TStringBuilder sb;
                sb << Config.GetUriPrefix();
                if (Config.GetUriPrefix().EndsWith("/") && request.GetUri().StartsWith("/")) {
                    sb << request.GetUri().substr(1);
                } else {
                    sb << request.GetUri();
                }
                const auto& suffix = Config.GetNecessaryUriSuffix();
                if (!!suffix && !sb.EndsWith(suffix)) {
                    if (sb.EndsWith("/") && suffix.StartsWith("/")) {
                        sb << suffix.substr(1);
                    } else {
                        sb << suffix;
                    }
                }
                request.SetTraceHeader(TLinkGuard::GetTraceLink());
                request.SetUri(sb);
                request.AddCgiData(Config.GetAdditionalCgi());
                return DoTuneRequest(request);
            } catch (...) {
                TFLEventLog::Error("exception")("message", CurrentExceptionMessage());
                return false;
            }
        }

        TString GetMockRequestDescriptor(NNeh::THttpRequest request) const {
            auto headers = request.GetHeaders();
            headers.erase("X-YaTraceId");
            headers.erase("X-YaRequestId");
            request.SetHeaders(std::move(headers));
            return request.GetDebugRequest();
        }

        template <class TRequest>
        void SetMockReply(const TRequest& r, const NNeh::THttpReply& reply) const {
            TString errorInfo;
            bool requestBuilt = false;
            NNeh::THttpRequest request;
            try {
                requestBuilt = r.BuildHttpRequest(request);
            } catch (...) {
                errorInfo = CurrentExceptionMessage();
                requestBuilt = false;
            }
            CHECK_WITH_LOG(requestBuilt) << errorInfo;
            TuneRequest(r, request);
            MockReplies[GetMockRequestDescriptor(request)] = reply;
        }

        template <class TRequest>
        bool PrepareNehRequest(const TRequest& r, NNeh::THttpRequest& request) const {
            bool requestBuilt = false;
            TString errorInfo;
            try {
                requestBuilt = r.BuildHttpRequest(request);
            } catch (...) {
                errorInfo = CurrentExceptionMessage();
                requestBuilt = false;
            }
            if (!requestBuilt) {
                NUtil::THttpReply report;
                report.SetErrorMessage("cannot_build_request: " + errorInfo);
                SignalProblem()("uri", request.GetUri())("code", "cannot_build_request")("req_type", request.GetRequestType());
                LogError(report, request, r, TDuration::Zero());
                return false;
            }
            if (!TuneRequest(r, request)) {
                NUtil::THttpReply report;
                report.SetErrorMessage("cannot_tune_request");
                SignalProblem()("uri", request.GetUri())("code", "cannot_tune_request")("req_type", request.GetRequestType());
                LogError(report, request, r, TDuration::Zero());
                return false;
            }
            if (r.HasTimeout()) {
                request.AddCgiData("timeout=" + ToString(r.GetTimeoutUnsafe().MicroSeconds()));
            }
            LogRequest(request, r);
            return true;
        }

        template <class TRequest>
        typename TRequest::TResponse SendRequest(const TRequest& r) const {
            NNeh::THttpRequest request;
            if (!PrepareNehRequest(r, request)) {
                typename TRequest::TResponse response;
                response.SetCode(HTTP_BAD_REQUEST);
                return response;
            }
            {
                auto it = MockReplies.find(GetMockRequestDescriptor(request));
                if (it != MockReplies.end()) {
                    return ProcessReport(r, request, it->second, 1, Now());
                }
                else if (UniversalReply.Defined()) {
                    return ProcessReport(r, request, *UniversalReply.Get(), 1, Now());
                }
            }
            const TInstant start = Now();
            NUtil::THttpReply report = r.HasTimeout() ? SendPreparedRequest(request, start + r.GetTimeoutUnsafe())
                            : SendPreparedRequest(request, TInstant::Zero());
            return ProcessReport(r, request, report, 1, start);
        }

        template <class TRequest, class TTarget = typename TRequest::TResponse>
        NThreading::TFuture<TTarget> SendRequestAsync(const TRequest& r) const {
            NNeh::THttpRequest nehRequest;
            if (!PrepareNehRequest(r, nehRequest)) {
                typename TRequest::TResponse response;
                response.SetCode(HTTP_BAD_REQUEST);
                return NThreading::MakeFuture(TTarget(response));
            }
            {
                auto it = MockReplies.find(GetMockRequestDescriptor(nehRequest));
                if (it != MockReplies.end()) {
                    return NThreading::MakeFuture(ProcessReport<TRequest, TTarget>(r, nehRequest, it->second, 1, Now()));
                }
                else if (UniversalReply.Defined()) {
                    return NThreading::MakeFuture(ProcessReport<TRequest, TTarget>(r, nehRequest, *UniversalReply.Get(), 1, Now()));
                }
            }
            const TInstant start = Now();
            auto reportFuture = SendPreparedRequestAsync(nehRequest, TInstant::Zero());
            return reportFuture.Apply([this, r, nehRequest, start](const NThreading::TFuture<NUtil::THttpReply>& reportFuture){
                auto report = reportFuture.GetValue();
                return ProcessReport<TRequest, TTarget>(r, nehRequest, report, 1, start);
            });
        }

        template <class TRequest, class TTarget = typename TRequest::TResponse, class... TArgs>
        NThreading::TFuture<TTarget> SendRequestAsync(TArgs... args) const {
            TRequest r(args...);
            return SendRequestAsync<TRequest, TTarget>(r);
        }

    private:
        NUtil::THttpReply SendPreparedRequest(NNeh::THttpRequest& request, const TInstant deadline) const {
            const TMaybe<NSimpleMeta::TConfig> reaskConfig = CustomizationContext ? CustomizationContext->GetRequestConfig(ApiName, request) : Nothing();
            if (reaskConfig) {
                request.SetConfigMeta(*reaskConfig);
            }
            return Impl->SendMessageSync(request, deadline);
        }

        NThreading::TFuture<NUtil::THttpReply> SendPreparedRequestAsync(NNeh::THttpRequest& request, const TInstant deadline) const {
            const TMaybe<NSimpleMeta::TConfig> reaskConfig = CustomizationContext ? CustomizationContext->GetRequestConfig(ApiName, request) : Nothing();
            if (reaskConfig) {
                request.SetConfigMeta(*reaskConfig);
            }
            return Impl->SendAsync(request, deadline);
        }

    protected:
        mutable TMap<TString, NNeh::THttpReply> MockReplies;

        void LogError(const NUtil::THttpReply& result, const NNeh::THttpRequest& request, const IServiceApiHttpRequest& baseReq, const TDuration d) const;
        void LogSuccess(const NUtil::THttpReply& result, const NNeh::THttpRequest& request, const IServiceApiHttpRequest& baseReq, const TDuration d) const;
        void LogRequest(const NNeh::THttpRequest& request, const IServiceApiHttpRequest& baseReq) const;

        static const TVector<double> TimeIntervals;

        template<class TRequest, class TTarget = typename TRequest::TResponse>
        typename TRequest::TResponse ProcessReport(const TRequest& request, const NNeh::THttpRequest& nehRequest, const NUtil::THttpReply& report, const ui32 attemption, const TInstant reqStart) const {
            auto gLogging = TFLRecords::StartContext()("req_id", request.GetRequestId());
            typename TRequest::TResponse result;
            const TDuration d = Now() - reqStart;
            result.ConfigureFromRequest(&request);
            result.SetRequestId(request.GetRequestId());
            result.SetRequestSession(THTTPRequestSession::BuildFromRequestResponse(nehRequest, report));
            result.ParseReply(report);
            const TString uriSignalId = request.GetSignalId() ? request.GetSignalId() : nehRequest.GetUri();
            TCSSignals::Signal("sender_api_attemption")("sender", ApiName)("uri", uriSignalId)("code", result.GetCode())("attemption", attemption)("req_type", nehRequest.GetRequestType())(request.BuildSignalTags())(result.BuildSignalTags());
            TCSSignals::HSignal("sender_api_duration", TimeIntervals)("sender", ApiName)("uri", uriSignalId)("code", result.GetCode())("req_type", nehRequest.GetRequestType())(d.MilliSeconds());
            if (result.IsSuccess()) {
                Signal()("uri", uriSignalId)("code", result.GetCode())("req_type", nehRequest.GetRequestType())(request.BuildSignalTags())(result.BuildSignalTags());
                LogSuccess(report, nehRequest, request, d);
            } else {
                if (!result.IsSuccessCode()) {
                    Signal()("uri", uriSignalId)("code", result.GetCode())("req_type", nehRequest.GetRequestType())(request.BuildSignalTags())(result.BuildSignalTags());
                } else if (!result.GetIsReportCorrect()) {
                    Signal()("uri", uriSignalId)("code", "incorrect_report")("req_type", nehRequest.GetRequestType())(request.BuildSignalTags())(result.BuildSignalTags());
                } else {
                    Signal()("uri", uriSignalId)("code", "unknown_problem")("req_type", nehRequest.GetRequestType())(request.BuildSignalTags())(result.BuildSignalTags());
                }
                LogError(report, nehRequest, request, d);
            }
            return TTarget(std::move(result));
        }
    };

}
