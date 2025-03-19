#include "abstract.h"
#include <util/system/tls.h>

namespace {
    Y_STATIC_THREAD(TString) TraceLink;
}

namespace NExternalAPI {

    using TObfuscatorKeyMap = NCS::NObfuscator::TObfuscatorKeyMap;
    using IObfuscator = NCS::NObfuscator::IObfuscator;

    namespace {
        TObfuscatorKeyMap MakeReplyObfuscatorKey(const NUtil::THttpReply& reply, const TString& requestType, const IServiceApiHttpRequest& baseReq, const TString& apiName)  {
            auto key = baseReq.GetObfuscatorKey();
            auto* contentType  = reply.GetHeaders().FindHeader("Content-Type");
            if (!!contentType) {
                key.SetType(IObfuscator::ToObfuscatorContentType(contentType->Value()));
                key.Add("Content-Type", contentType->Value());
            }
            key.Add("Uri", requestType);
            key.Add("Code", ToString(reply.Code()));
            key.Add("Api-Name", apiName);
            return key;
        }

        TObfuscatorKeyMap MakeRequestObfuscatorKey(const NNeh::THttpRequest& request, const TString& requestType, const IServiceApiHttpRequest& baseReq, const TString& apiName) {
            auto key = baseReq.GetObfuscatorKey();
            const auto& headers = request.GetHeaders();
            auto contentType  = headers.find("Content-Type");
            if (contentType != headers.end()) {
                key.SetType(IObfuscator::ToObfuscatorContentType(contentType->second));
                key.Add("Content-Type", contentType->second);
            }
            key.Add("Uri", requestType);
            key.Add("Api-Name", apiName);
            return key;
        }
    }

    TSender::TLinkGuard::TLinkGuard(const TString& trace) {
        ::TraceLink.Get() = trace;
    }

    TString TSender::TLinkGuard::GetTraceLink() {
        return ::TraceLink.Get();
    }

    TSender::TLinkGuard::~TLinkGuard() {
        ::TraceLink.Get() = "";
    }

    const TVector<double> TSender::TimeIntervals = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 300, 400, 500, 1000, 2000, 3000, 4000, 5000, 7000, 10000 };

    void TSender::LogError(const NUtil::THttpReply& result, const NNeh::THttpRequest& request, const IServiceApiHttpRequest& baseReq, const TDuration d) const {
        if (!Config.GetLogEventsGlobal()) {
            return;
        }
        TFLEventLog::TLogWriterContext logEvent = TFLEventLog::ModuleLog("ServiceApi", TLOG_INFO);
        logEvent("http_code", result.Code())
            ("uri", request.GetUri())
            ("trace_id", request.GetTraceHeader())
            ("api", ApiName)
            ("req_id", baseReq.GetRequestId())
            ("duration", d.MilliSeconds());
        const TString obfuscated = ObfuscatorManager.Obfuscate(MakeReplyObfuscatorKey(result, request.GetUri(), baseReq, ApiName), result.Content());
        logEvent.AddEscaped("response", obfuscated, Config.GetResponseSizeLimit());

        for (auto&& i : baseReq.GetLinks()) {
            logEvent(i.first, i.second);
        }
        for (auto&& i : baseReq.GetReplyLinks()) {
            auto h = result.GetHeaders().FindHeader(i);
            if (h) {
                logEvent(i, h->Value());
            }
        }
        for (auto&& i : Config.GetLogHeadersReply()) {
            auto h = result.GetHeaders().FindHeader(i);
            if (h) {
                logEvent(i, h->Value());
            }
        }
        for (auto&& event : baseReq.TuneLogEvent(logEvent)) {
            TFLEventLog::Log(event);
        }
        TCSSignals::Signal(ApiName + "." + baseReq.GetSignalId() + ".errors");
    }

    void TSender::LogSuccess(const NUtil::THttpReply& result, const NNeh::THttpRequest& request, const IServiceApiHttpRequest& baseReq, const TDuration d) const {
        if (!Config.GetLogEventsGlobal()) {
            return;
        }
        TFLEventLog::TLogWriterContext logEvent = TFLEventLog::ModuleLog("ServiceApi", TLOG_INFO);
        logEvent("http_code", result.Code())
            ("api", ApiName)
            ("uri", request.GetUri())
            ("trace_id", request.GetTraceHeader())
            ("req_id", baseReq.GetRequestId())
            ("duration", d.MilliSeconds());
        const TString responseStr = Config.GetLogEventsResponse() ? result.Content() : "NOT_CONFIGURED_RESPONSE_WRITING";
        const TString obfuscated = ObfuscatorManager.Obfuscate(MakeReplyObfuscatorKey(result, request.GetUri(), baseReq, ApiName), responseStr);
        logEvent.AddEscaped("response", obfuscated, Config.GetResponseSizeLimit());
        for (auto&& i : baseReq.GetLinks()) {
            logEvent(i.first, i.second);
        }
        for (auto&& i : baseReq.GetReplyLinks()) {
            auto h = result.GetHeaders().FindHeader(i);
            if (h) {
                logEvent(i, h->Value());
            }
        }
        for (auto&& i : Config.GetLogHeadersReply()) {
            auto h = result.GetHeaders().FindHeader(i);
            if (h) {
                logEvent(i, h->Value());
            }
        }

        for (auto&& event : baseReq.TuneLogEvent(logEvent)) {
            TFLEventLog::Log(event);
        }

        TCSSignals::Signal(ApiName + "." + baseReq.GetSignalId() + ".success");
    }

    void TSender::LogRequest(const NNeh::THttpRequest& request, const IServiceApiHttpRequest& baseReq) const {
        if (!Config.GetLogEventsGlobal()) {
            return;
        }
        TStringStream ss;
        ss << request.GetRequest(true, Config.GetHiddenCgiParameters()) << " " << baseReq.GetBodyForLog(request);
        TFLEventLog::TLogWriterContext logEvent = TFLEventLog::ModuleLog("ServiceApi", TLOG_INFO);
        logEvent("uri", request.GetUri())("req_type", request.GetRequestType())
            ("api", ApiName)
            ("trace_id", request.GetTraceHeader())
            ("req_id", baseReq.GetRequestId());

        logEvent.AddEscaped("request", ObfuscatorManager.Obfuscate(MakeRequestObfuscatorKey(request, request.GetUri(), baseReq, ApiName), ss.Str()), Config.GetRequestSizeLimit());
        for (auto&& i : baseReq.GetLinks()) {
            logEvent(i.first, i.second);
        }

        for (auto&& i : request.GetHeaders()) {
            if (Config.GetLogHeadersRequest().contains("*") || Config.GetLogHeadersRequest().contains(i.first)) {
                logEvent(i.first, i.second);
            }
        }

        for (auto&& event : baseReq.TuneLogEvent(logEvent)) {
            TFLEventLog::Log(event);
        }
    }

    void TSenderConfig::Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy) {
        TBase::Init(section, requestPolicy);
        UriPrefix = section->GetDirectives().Value("UriPrefix", UriPrefix);
        NecessaryUriSuffix = section->GetDirectives().Value("NecessaryUriSuffix", NecessaryUriSuffix);
        AdditionalCgi = section->GetDirectives().Value("AdditionalCgi", AdditionalCgi);
        MaxInFly = section->GetDirectives().Value("MaxInFly", MaxInFly);
        LogEventsGlobal = section->GetDirectives().Value("LogEventsGlobal", LogEventsGlobal);
        LogEventsResponse = section->GetDirectives().Value("LogEventsResponse", LogEventsResponse);
        auto children = section->GetAllChildren();
        {
            auto it = children.find("Headers");
            if (it != children.end()) {
                for (auto&& h : it->second->GetDirectives()) {
                    Headers.emplace(h.first, h.second);
                }
            }
        }
        StringSplitter(section->GetDirectives().Value<TString>("LogHeadersReply", "")).SplitBySet(", ").SkipEmpty().Collect(&LogHeadersReply);
        LogHeadersReply.emplace("X-Market-Req-Id");
        LogHeadersReply.emplace("X-YaRequestId");

        StringSplitter(section->GetDirectives().Value<TString>("HiddenCgiParameters", "")).SplitBySet(", ").SkipEmpty().Collect(&HiddenCgiParameters);
        StringSplitter(section->GetDirectives().Value<TString>("LogHeadersRequest", "")).SplitBySet(", ").SkipEmpty().Collect(&LogHeadersRequest);
        LogHeadersRequest.emplace("X-YaRequestId");
        {
            auto itCustom = children.find("Customization");
            if (itCustom != children.end()) {
                Customizer.Init(itCustom->second);
            }
        }
    }

    void TSenderConfig::ToString(IOutputStream& os) const {
        TBase::ToString(os);
        os << "UriPrefix: " << UriPrefix << Endl;
        os << "NecessaryUriSuffix: " << NecessaryUriSuffix << Endl;
        os << "AdditionalCgi: " << AdditionalCgi << Endl;
        os << "MaxInFly: " << MaxInFly << Endl;
        os << "LogEventsGlobal: " << LogEventsGlobal << Endl;
        os << "LogEventsResponse: " << LogEventsResponse << Endl;
        os << "LogHeadersReply: " << JoinSeq(", ", LogHeadersReply) << Endl;
        os << "LogHeadersRequest: " << JoinSeq(", ", LogHeadersRequest) << Endl;
        os << "HiddenCgiParameters: " << JoinSeq(", ", HiddenCgiParameters) << Endl;

        os << "<Headers>" << Endl;
        for (auto&& i : Headers) {
            os << i.first << ": " << i.second << Endl;
        }
        os << "</Headers>" << Endl;
        if (!!Customizer) {
            os << "<Customization>" << Endl;
            Customizer.ToString(os);
            os << "</Customization>" << Endl;
        }
    }
}
