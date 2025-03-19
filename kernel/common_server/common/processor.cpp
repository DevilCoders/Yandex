#include "processor.h"

#include <kernel/common_server/notifications/abstract/abstract.h>
#include <kernel/common_server/library/geometry/coord.h>
#include <kernel/common_server/library/network/data/data.h>
#include <kernel/common_server/util/coded_exception.h>
#include <kernel/common_server/util/events_rate_calcer.h>

#include <library/cpp/regex/pcre/regexp.h>

#include <util/system/hostname.h>
#include <util/generic/guid.h>

namespace {
    NCS::NObfuscator::IObfuscator::EContentType GetType(const TString& overridenContentType) {
        if (!overridenContentType.Empty()) {
            return NCS::NObfuscator::IObfuscator::ToObfuscatorContentType(overridenContentType);
        }
        return NCS::NObfuscator::IObfuscator::EContentType::Json;
    }
}

IRequestProcessor::IRequestProcessor(const IRequestProcessorConfig& config, IReplyContext::TPtr context, const IBaseServer* server)
    : TBase(server, context ? context->GetUri() : config.GetHandlerName())
    , Context(context)
    , AccessControlAllowOrigin(config.GetAccessControlAllowOrigin())
    , DebugModeFlag(Context ? Context->GetCgiParameters().Has("pron", "debug_info") : false)
    , RateLimitFreshness(config.GetRateLimitFreshness())
    , MaxRequestSize(config.GetMaxRequestSize())
{
}

TMaybe<NCS::NScheme::THandlerResponse> IRequestProcessor::BuildDefaultScheme(const IBaseServer& /*server*/) const {
    NCS::NScheme::THandlerResponse result;
    result.SetDescription("default scheme");
    NCS::NScheme::TScheme& resultScheme = result.AddContent();
    resultScheme.Add<TFSArray>("error_details").SetElement<TFSString>();
    resultScheme.Add<TFSString>("code");
    resultScheme.Add<TFSString>("message");
    auto& fsScheme = resultScheme.Add<TFSStructure>("details").SetStructure();
    fsScheme.Add<TFSNumeric>("http_code");
    fsScheme.Add<TFSString>("debug_message");
    fsScheme.Add<TFSString>("details");
    fsScheme.Add<TFSArray>("errors").SetElement<TFSString>();
    fsScheme.Add<TFSString>("meta_code");
    return result;
}

TMaybe<NCS::NScheme::THandlerScheme> IRequestProcessor::BuildScheme(const NCS::TPathHandlerInfo& pathInfo, const IBaseServer& server) const {
    NCS::NScheme::THandlerScheme result(pathInfo.GetPatternInfo());
    if (!DoFillHandlerScheme(result, server)) {
        return Nothing();
    }
    TMaybe<NCS::NScheme::THandlerResponse> defaultReply = BuildDefaultScheme(server);
    if (!!defaultReply) {
        result.AddDefaultReply(*defaultReply);
    }
    return result;
}

IReplyContext::TPtr IRequestProcessor::GetContext() const {
    return Context;
}

IThreadPool* IRequestProcessor::GetHandler() const {
    return BaseServer->GetRequestsHandler("default");
}

namespace {
    TRWMutex RateMeterMutex;
    TMap<TString, TEventRate<100000>> RateMeters;
}

bool IRequestProcessor::CheckRequestsRateLimit() const {
    if (AtomicGet(RateLimitRefresh) + RateLimitFreshness.Seconds() < Now().Seconds()) {
        const ui32 rateLimit = BaseServer->GetSettings().GetHandlerValueDef<ui32>(GetHandlerName(), "rate_limit", BaseServer->GetSettings().GetHandlerValueDef<ui32>("default", "rate_limit", 0));
        AtomicSet(RateLimit, rateLimit);
        AtomicSet(RateLimitRefresh, Now().Seconds());
    }
    if (!AtomicGet(RateLimit)) {
        return true;
    }
    TEventRate<100000>* er;
    {
        TReadGuard rg(RateMeterMutex);
        auto it = RateMeters.find(GetHandlerName());
        if (it != RateMeters.end()) {
            er = &it->second;
        };
    }
    {
        TWriteGuard rg(RateMeterMutex);
        er = &RateMeters.emplace(GetHandlerName(), TEventRate<100000>()).first->second;
    }
    TInstant start;
    TInstant finish;
    ui64 eventsCount;
    er->GetInterval(TDuration::Seconds(1), start, finish, eventsCount);
    if ((i64)eventsCount < AtomicGet(RateLimit)) {
        er->Hit();
        return true;
    }
    return false;
}

namespace {
    NCS::NObfuscator::TObfuscatorKeyMap MakeRequestBodyObfuscatorKey(const TString& handlerName)  {
        auto key = NCS::NObfuscator::TObfuscatorKeyMap("application/json");
        key.Add("obfuscated_entity", "request_body");
        key.Add("handler_type", handlerName);
        return key;
    }
}

void IRequestProcessor::InitJsonData() const {
    if (!JsonData) {
        ReqCheckCondition(GetContentType().StartsWith("application/json") || GetContentType() == "", ConfigHttpStatus.SyntaxErrorStatus, "http.request.json.content-type.incorrect");
        TBlob postData = Context->GetBuf();
        TMemoryInput inp(postData.Data(), postData.Size());
        auto json = MakeHolder<NJson::TJsonValue>(NJson::JSON_NULL);
        try {
            ReqCheckCondition(postData.Empty() || NJson::ReadJsonTree(&inp, json.Get(), true), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "http.request.content.incorrect_json");
            ReqCheckCondition(BaseServer->GetObfuscatorManager().ObfuscateInplace(MakeRequestBodyObfuscatorKey(GetHandlerName()), *json), ConfigHttpStatus.SyntaxErrorStatus, "http.request.obfuscator.failed");
            JsonData.Reset(json.Release());
        } catch (...) {
            ReqCheckCondition(false, ConfigHttpStatus.SyntaxErrorStatus, "http.request.json.content.incorrect");
        }
    }
}

bool IRequestProcessor::DoFillHandlerScheme(NCS::NScheme::THandlerScheme& /*scheme*/, const IBaseServer& /*server*/) const {
    return false;
}

bool IRequestProcessor::CheckCache(const TString& hash, TJsonReport::TGuard& g, const TInstant reqActuality) const {
    NJson::TJsonValue report;
    auto cacheResult = BaseServer->GetCache("handler-" + GetHandlerName()).GetData(hash, report, reqActuality);
    if (cacheResult == TCacheWithAge<TString, NJson::TJsonValue>::ECachedValue::HasData) {
        g.MutableReport().SetExternalReport(std::move(report));
        return true;
    }
    return false;
}

void IRequestProcessor::UpdateCache(const TString& hash, TJsonReport::TGuard& g) {
    BaseServer->GetCache("handler-" + GetHandlerName()).Refresh(hash, g.MutableReport().GetReport());
}

IFrontendReportBuilder::TCtx IRequestProcessor::GetReportContext() const {
    return {
        Context,
        AccessControlAllowOrigin,
        GetHandlerName()
    };
}

bool IRequestProcessor::HasJsonData() const {
    try {
        InitJsonData();
        return !!JsonData;
    } catch (...) {
        return false;
    }
}

void IRequestProcessor::Process() {
    auto report = MakeAtomicShared<TJsonReport>(GetReportContext());
    report->SetRecordsLimit(Min<ui32>(report->GetRecordsLimit(), BaseServer->GetSettings().GetValueDef<ui32>("defaults.event_log_records_limit", 1000)));
    report->SetLogLineLimit(BaseServer->GetSettings().GetValueDef<ui32>("limits.response_log_line_limit", 1024 * 256));
    const auto& configHttpCodes = BaseServer->GetHttpStatusManagerConfig();
    TFLEventLog::TContextGuard cGuard(report.Get());
    TJsonReport::TGuard g(report, configHttpCodes.UnknownErrorStatus);

    if (!CheckRequestsRateLimit()) {
        g.SetCode(configHttpCodes.TooManyRequestsStatus);
        AfterFinish(g.GetCode());
        return;
    }

    if (MaxRequestSize && GetRawData().size() > MaxRequestSize) {
        g.SetCode(configHttpCodes.RequestSizeIsTooBig);
        AfterFinish(g.GetCode());
        return;
    }

    IFrontendNotifier::TPtr notifier = BaseServer->GetNotifier(GetHandlerSettingDef<TString>("problems_notifier", ""));
    NCS::NLogging::TDefaultLogsAccumulator dla;
    auto logGuard = TFLRecords::StartContext()("handler", GetHandlerName());
    try {
        DoProcess(g);
        const auto minReplyTime = GetHandlerSettingDef<TDuration>("min_reply_time", TDuration::Zero());
        const auto processingTime = Now() - Context->GetRequestStartTime();
        if (processingTime < minReplyTime) {
            Sleep(minReplyTime - processingTime);
        }
    } catch (const TCodedException& e) {
        report->AddReportElement("error_details", dla.GetJsonReport());
        g.SetCode(e);
        if (!!notifier) {
            notifier->Notify(IFrontendNotifier::TMessage(Context->GetUri(), e.GetDetailedReport().GetStringRobust() + "\n" + e.GetDebugMessage()));
        }
    } catch (const yexception& e) {
        report->AddReportElement("error", e.what());
        report->AddReportElement("error_details", dla.GetJsonReport());

        g.SetCode(configHttpCodes.UnknownErrorStatus);
        if (!!notifier) {
            notifier->Notify(IFrontendNotifier::TMessage(Context->GetUri(), e.what()));
        }
    } catch (...) {
        report->AddReportElement("error", CurrentExceptionMessage());
        report->AddReportElement("error_details", dla.GetJsonReport());
        g.SetCode(configHttpCodes.UnknownErrorStatus);
        if (!!notifier) {
            notifier->Notify(IFrontendNotifier::TMessage(Context->GetUri(), CurrentExceptionMessage()));
        }
    }

    auto key = GetObfuscatorKey();
    key.Add("Uri", Context->GetUri());
    key.SetType(GetType(report->GetOverridenContentType()));
    report->SetObfuscator(BaseServer->GetObfuscatorManager().GetObfuscatorFor(key));

    AfterFinish(g.GetCode());
}

TMaybe<TGeoCoord> IRequestProcessor::GetUserLocation(const TString& cgiUserLocation) const {
    CHECK_WITH_LOG(Context);
    if (cgiUserLocation) {
        const TMaybe<TGeoCoord> src = GetValue<TGeoCoord>(Context->GetCgiParameters(), cgiUserLocation, /*required=*/false);
        if (src) {
            return *src;
        }
    }
    const TBaseServerRequestData& rd = Context->GetBaseRequestData();
    TStringBuf lat = rd.HeaderInOrEmpty("Lat");
    TStringBuf lon = rd.HeaderInOrEmpty("Lon");
    if (!lat || !lon) {
        return {};
    }

    auto latitude = ParseValue<double>(lat);
    auto longitude = ParseValue<double>(lon);
    return TGeoCoord(longitude, latitude);
}

IRequestProcessorConfig::IRequestProcessorConfig(const TString& handlerName)
    : HandlerName(handlerName)
{
}

IRequestProcessorConfig::~IRequestProcessorConfig() {
}

void IRequestProcessorConfig::CheckServerForProcessor(const IBaseServer* /*server*/) const {
}

IRequestProcessor::TPtr IRequestProcessorConfig::ConstructProcessor(IReplyContext::TPtr context, const IBaseServer* server, const TMap<TString, TString>& selectionParams) const {
    IRequestProcessor::TPtr result = DoConstructProcessor(context, server);
    if (!!result) {
        result->SetUrlParams(selectionParams);
    }
    return result;
}

bool IRequestProcessorConfig::Init(const TYandexConfig::Section* section) {
    const TYandexConfig::Directives& directives = section->GetDirectives();
    ProcessorType = directives.Value("ProcessorType", ProcessorType);
    AdditionalCgi = directives.Value("AdditionalCgi", AdditionalCgi);
    OverrideCgi = directives.Value("OverrideCgi", OverrideCgi);
    OverrideCgiPart = directives.Value("OverrideCgiPart", OverrideCgiPart);
    OverridePost = directives.Value("OverridePost", OverridePost);
    AccessControlAllowOrigin = directives.Value("Access-Control-Allow-Origin", AccessControlAllowOrigin);
    AccessControlAllowOriginRegEx = AccessControlAllowOrigin ? MakeHolder<TRegExMatch>(AccessControlAllowOrigin) : nullptr;
    RequestTimeout = directives.Value("RequestTimeout", RequestTimeout);
    RateLimitFreshness = directives.Value("RateLimitFreshness", RateLimitFreshness);
    if (!directives.GetValue("ThreadPoolName", ThreadPoolName)) {
        ThreadPoolName = directives.Value("ThreadPool", ThreadPoolName);
    }
    MaxRequestSize = directives.Value("MaxRequestSize", MaxRequestSize);
    const TVector<TString> params = StringSplitter(directives.Value<TString>("DefaultUrlParams", "")).SplitBySet(";").SkipEmpty().ToList<TString>();
    for (auto&& i : params) {
        TStringBuf sb(i.data(), i.size());
        TStringBuf l;
        TStringBuf r;
        AssertCorrectConfig(sb.TrySplit('=', l, r), "incorrect additional url params %s", sb.data());
        AssertCorrectConfig(DefaultUrlParams.emplace(l, r).second, "duplication url params %s", sb.data());
    }
    return DoInit(section);
}

void IRequestProcessorConfig::ToString(IOutputStream& os) const {
    os << "ProcessorType: " << ProcessorType << Endl;
    os << "AdditionalCgi: " << AdditionalCgi << Endl;
    os << "OverrideCgi: " << OverrideCgi << Endl;
    os << "OverrideCgiPart: " << OverrideCgiPart << Endl;
    os << "OverridePost: " << OverridePost << Endl;
    os << "RequestTimeout: " << RequestTimeout << Endl;
    os << "RateLimitFreshness: " << RateLimitFreshness << Endl;
    os << "ThreadPoolName: " << ThreadPoolName << Endl;
    os << "MaxRequestSize: " << MaxRequestSize << Endl;
    TStringBuilder sb;
    for (auto&& i : DefaultUrlParams) {
        sb << i.first << "=" << i.second << ";";
    }
    os << "DefaultUrlParams: " << sb << Endl;
    DoToString(os);
}
