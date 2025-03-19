#include "replier.h"

#include <kernel/common_server/library/logging/events.h>

#include <kernel/common_server/library/cgi_corrector/abstract.h>
#include <kernel/common_server/library/network/data/data.h>
#include <kernel/common_server/util/algorithm/container.h>

#include <kernel/common_server/util/coded_exception.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/reqid/reqid.h>

namespace {
    TMutex HandlersCounterMutex;
    TMap<TString, i32> HandlersCounter;
}

TReplier::THandlersCounterGuard::THandlersCounterGuard(const TString& key, const TString& metric)
    : Key(key + "." + metric)
    , Handler(key)
{
    TGuard<TMutex> g(HandlersCounterMutex);
    HandlersCounter[Key] = HandlersCounter[Key] + 1;
    TCSSignals::LSignal("http_handlers")("metric", "threads_count")("handler", Handler)(HandlersCounter[Key]);
}

TReplier::THandlersCounterGuard::~THandlersCounterGuard()
{
    TGuard<TMutex> g(HandlersCounterMutex);
    HandlersCounter[Key] = HandlersCounter[Key] - 1;
    TCSSignals::LSignal("http_handlers")("metric", "threads_count")("handler", Handler)(HandlersCounter[Key]);
}

void TReplier::MakeErrorPage(ui32 code, const TString& error) {
    Y_ASSERT(!!Context);
    TJsonReport report(Context, GetHandlerName());
    report.AddReportElement("error", error);
    report.Finish(code);
}

TReplier::TReplier(IReplyContext::TPtr context, const TBaseServerConfig* config, const IBaseServer* server)
    : IHttpReplier(context, &config->GetHttpStatusManagerConfig())
    , Config(config)
    , Server(server)
{
    HandlerName = context->GetUri(Config->GetRequestProcessingConfig().GetDefaultType());

    const TBaseServerRequestData& rd = Context->GetBaseRequestData();

    const auto traceId = rd.HeaderInOrEmpty("X-YaTraceId");
    const auto link = JoinStrings(SplitString(CreateGuidAsString(), "-"), "");

    TString robustTraceId;
    if (traceId) {
        robustTraceId = TString{ traceId };
    } else {
        robustTraceId = JoinStrings(SplitString(CreateGuidAsString(), "-"), "");
    }

    auto evLogRecord = TFLEventLog::ModuleLog("access")("source", GetHandlerName())("link", link)("trace_id", robustTraceId)("_type", "request");
    Context->Init(evLogRecord, Server->GetObfuscatorManager());
    Context->SetLink(link);
    Context->SetTraceId(robustTraceId);

    Context->AddReplyInfo("X-YaRequestId", link);
    Context->AddReplyInfo("X-YaTraceId", robustTraceId);

    if (!!Server->HasProcessorInfo(GetHandlerName())) {
        SignalGuard = MakeHolder<THandlersCounterGuard>(GetHandlerName(), "total-requests");
    }
    for (const auto& header: Config->GetRequestProcessingConfig().GetHeadersToForward()) {
        if (TStringBuf value = rd.HeaderInOrEmpty(header)) {
            Context->AddReplyInfo(header, TString{value});
        }
    }
}

TReplier::~TReplier() {
}

double TReplier::GetDefaultKffWaitingAvailable() const {
    return Server->GetSettings().GetValueDef<double>({ "defaults.kff_waiting.handlers." + GetHandlerName(), "defaults.kff_waiting.handlers.*" }, "", 0.9);
}

TDuration TReplier::GetDefaultTimeout() const {
    TRequestProcessorConfigContainer processorGenerator = Server->GetProcessorInfo(GetHandlerName());
    TDuration defDuration = IRequestProcessorConfig::NotInitializedTimeout;
    if (!!processorGenerator) {
        defDuration = processorGenerator->GetRequestTimeout();
    }
    return Server->GetSettings().GetValueDef<TDuration>({ "defaults.timeouts.handlers." + GetHandlerName(), "defaults.timeouts.handlers.*" }, "", defDuration);
}

void TReplier::OnRequestExpired() {
    if (Server->HasProcessorInfo(GetHandlerName())) {
        TCSSignals::Signal("http_handlers")("handler", GetHandlerName())("metric", "reply.count")("code", "request_expired");
    } else {
        TFLEventLog::Signal("http_handlers")("&handler", "unknown")("&metric", "reply.count")("&code", "request_expired")("original_handler", GetHandlerName());
    }
    auto gContext = TFLRecords::StartContext().Source(GetHandlerName());
    Context->Init(TFLEventLog::ModuleLog("timeout", TLOG_ERR)("meta_code", HTTP_GATEWAY_TIME_OUT)("http_code", HTTP_GATEWAY_TIME_OUT)("error", "true"), Server->GetObfuscatorManager());
}

bool TReplier::InitializeEncryptor(IReplyContext::TPtr context, TString& errorCode) const {
    ui32 secretVersion = 0;
    TString secretHeader(context->GetBaseRequestData().HeaderInOrEmpty("SecretVersion"));
    if (!!secretHeader && !TryFromString<ui32>(secretHeader, secretVersion)) {
        errorCode = "incorrect secret header";
        return false;
    }

    const ui32 minSecretVersion = Server->GetSettings().GetHandlerValueDef<ui32>(GetHandlerName(), "report_secret.version", 0);
    if (minSecretVersion > 0) {
        if (secretVersion < minSecretVersion) {
            errorCode = "too old secret";
            return false;
        }
    }
    if (secretVersion == 0) {
        context->AddReplyInfo("SecretVersion", "0");
        return true;
    }

    const TString secretData = Server->GetSettings().GetHandlerValueDef<TString>(GetHandlerName(), "report_secret." + secretHeader + ".data", "");
    if (!secretData) {
        if (minSecretVersion == 0) {
            context->AddReplyInfo("SecretVersion", "0");
            return true;
        }
        errorCode = "no secret";
        return false;
    }
    context->SetReportEncryptor(MakeHolder<TReportEncryptor>(Base64Decode(secretData), secretVersion));
    return true;
}

void TReplier::DoSearchAndReply() {
    auto eventLogContext = TFLRecords::StartContext().Source(GetHandlerName())("&handler", GetHandlerName());
    auto gLinks = NExternalAPI::TSender::InitLinks(Context->GetTraceId());
    eventLogContext("link", Context->GetLink())("trace_id", Context->GetTraceId());
    try {
        if (!Server->GetSettings().GetHandlerValueDef(GetHandlerName(), "base_enabled", true)) {
            throw TCodedException(Config->GetHttpStatusManagerConfig().ServiceUnavailable) << "Disabled handler: " << GetHandlerName();
        }
        TRequestProcessorConfigContainer processorGenerator = Server->GetProcessorInfo(GetHandlerName());
        if (!!processorGenerator) {
            TCSSignals::Signal("http_handlers")("handler", GetHandlerName())("metric", "access.count");
        } else {
            TFLEventLog::Signal("http_handlers")("&handler", "__incorrect_handler")("&metric", "unknown.access.count")("handler_name_original", GetHandlerName());
        }

        const TString whitelistStr = Server->GetSettings().GetHandlerValueDef<TString>(GetHandlerName(), "whitelist", "");
        if (whitelistStr) {
            TSet<TString> whitelist = MakeSet(SplitString(whitelistStr, ","));
            TString clientIp(NUtil::GetClientIp(Context->GetBaseRequestData()));
            Y_ENSURE_EX(whitelist.contains(clientIp), TCodedException(Config->GetHttpStatusManagerConfig().PermissionDeniedStatus));
        }

        if (!processorGenerator) {
            throw TCodedException(Config->GetHttpStatusManagerConfig().SyntaxErrorStatus) << "cannot construct processor generator for " << GetHandlerName();
        }
        THandlersCounterGuard signalGuard(GetHandlerName(), "in-progress");

        TString errorCode;
        Y_ENSURE_EX(InitializeEncryptor(Context, errorCode), TCodedException(Config->GetHttpStatusManagerConfig().UserErrorState) << errorCode);

        TString customCompression(Context->GetBaseRequestData().HeaderInOrEmpty("Use-Custom-Compression"));
        if (IsTrue(customCompression)) {
            Context->SetReportCompressor(MakeHolder<TReportCompressor>(TReportCompressor::EType::GZip));
        }


        if (!!processorGenerator->GetOverrideCgi() || !!processorGenerator->GetAdditionalCgi()) {
            const TString cgiStr = !!processorGenerator->GetOverrideCgi() ? processorGenerator->GetOverrideCgi() : processorGenerator->GetAdditionalCgi();
            TCgiParameters originalCgi = Context->MutableCgiParameters();
            if (!!processorGenerator->GetOverrideCgi()) {
                Context->MutableCgiParameters().clear();
                auto reqid = originalCgi.find("reqid");
                if (reqid != originalCgi.end()) {
                    Context->MutableCgiParameters().insert(*reqid);
                }
            }
            TCgiParameters cgi;
            cgi.Scan(cgiStr);
            for (auto&& i : cgi) {
                if (i.second.StartsWith("$") && i.second.EndsWith("$") && i.second.size() > 1) {
                    Context->MutableCgiParameters().InsertUnescaped(i.first, originalCgi.Get(i.second.substr(1, i.second.size() - 2)));
                } else {
                    Context->MutableCgiParameters().InsertUnescaped(i.first, i.second);
                }
            }
        }
        if (!!processorGenerator->GetOverrideCgiPart()) {
            const TString cgiStr = processorGenerator->GetOverrideCgiPart();
            TCgiParameters originalCgi = Context->MutableCgiParameters();
            TCgiParameters cgi;
            cgi.Scan(cgiStr);
            for (auto&& i : cgi) {
                if (i.second.StartsWith("$") && i.second.EndsWith("$") && i.second.size() > 1) {
                    Context->MutableCgiParameters().ReplaceUnescaped(i.first, originalCgi.Get(i.second.substr(1, i.second.size() - 2)));
                } else {
                    Context->MutableCgiParameters().ReplaceUnescaped(i.first, i.second);
                }
            }
        }
        if (!!processorGenerator->GetOverridePost()) {
            Context->SetBuf(processorGenerator->GetOverridePost());
        }
        Processor = processorGenerator->ConstructProcessor(Context, Server, processorGenerator.GetSelectionParams());
        if (!Processor) {
            throw TCodedException(Config->GetHttpStatusManagerConfig().SyntaxErrorStatus) << "cannot construct processor for " << GetHandlerName();
        }

        Processor->Process();
    } catch (const TCodedException& cException) {
        TJsonReport report(Context, GetHandlerName());
        report.AddReportElement("error", cException.what());
        report.Finish(cException.GetCode());
        return;
    } catch (...) {
        TJsonReport report(Context, GetHandlerName());
        report.AddReportElement("error", CurrentExceptionMessage());
        report.Finish(Config->GetHttpStatusManagerConfig().UserErrorState);
        return;
    }
}

void TReplier::OnQueueFailure() {
    TFLEventLog::Signal("http_handlers")("&metric", "queue_failure")("&handler", GetHandlerName()).Error();
    MakeErrorPage(HTTP_SERVICE_UNAVAILABLE, "queue_failure");
}

IThreadPool* TReplier::DoSelectHandler() {
    TRequestProcessorConfigContainer processorGenerator = Server->GetProcessorInfo(GetHandlerName());
    Y_ENSURE(!!processorGenerator, yexception() << "Incorrect processor for request: " << GetHandlerName());
    const TString threadPoolName = processorGenerator->GetThreadPoolName() ? processorGenerator->GetThreadPoolName() : "default";
    if (Context) {
        Context->SetThreadPoolId(threadPoolName);
    }
    IThreadPool* handler = Server->GetRequestsHandler(threadPoolName);
    TFLEventLog::Signal("thread_pool_usage")("&handler", GetHandlerName())("&code", "select_handler")("&pool_id", threadPoolName).Debug()("initialized", (bool)handler);
    Y_ENSURE(!!handler, yexception() << "Incorrect handler for request: " << GetHandlerName() << "?" << (Context ? Context->GetCgiParameters().Print() : "unknown"));
    return handler;
}
