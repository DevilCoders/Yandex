#include "http.h"

#include "http_common.h"
#include "log.h"
#include "master.h"
#include "master_profiler.h"
#include "master_target_graph.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/http_static.h>
#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/communism/util/http_util.h>

#include <library/cpp/http/misc/httpdate.h>
#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/mime/types/mime.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/system/fstat.h>

TMaybe<TString> GetHeader(const THttpHeaders& headers, const TString& value) {
    for (THttpHeaders::TConstIterator h = headers.Begin(); h != headers.End(); ++h) {
        if (AsciiEqualsIgnoreCase(h->Name(), value)) {
            return h->Value();
        }
    }

    return Nothing();
}


TString TAuthorizationCallbacks::GetKey(const TString& key) const {
    return key;
}

TAuthorizationInfo* TAuthorizationCallbacks::CreateObject(const TString& key) const {
    TString cookie;
    TString token;

    if (key.StartsWith("Session_id=")) {
        cookie = key;
    }
    if (key.StartsWith("OAuth")) {
        token = key;
    }

    TAuthorizationInfo authInfo = GetUserWithAuthorization(cookie, token);

    return new TAuthorizationInfo(authInfo);
}

TAuthorizationInfo TAuthorizationCallbacks::GetUserWithAuthorization(const TString& cookie, const TString& token) const {
    static const TString WEBAUTH_SERVER = "https://webauth.yandex-team.ru";
    TKeepAliveHttpClient webauthHttpClient(WEBAUTH_SERVER, 443);

    TKeepAliveHttpClient::THeaders webauthHeaders;
    if (token) {
        webauthHeaders["Authorization"] = token;
    }
    if (cookie) {
        webauthHeaders["Cookie"] = cookie;
    }

    TStringBuilder webauthRequestPath;
    webauthRequestPath << "/auth_request";
    if (MasterOptions.IdmRole) {
        webauthRequestPath << "?idm_role=" << MasterOptions.IdmRole;
    }
    TStringStream webauthResponse;
    THttpHeaders webauthResponseHeaders;

    LOG("Requesting " << WEBAUTH_SERVER << webauthRequestPath);
    try {
        webauthHttpClient.DoGet(webauthRequestPath, &webauthResponse, webauthHeaders, &webauthResponseHeaders);
        LOG("Webauth response is [" << StripInPlace(webauthResponse.Str()) << "]");
    } catch (const THttpRequestException& e) {
        ERRORLOG("Unknown authorization problem\nHttp code " << e.GetStatusCode() << "\n" << webauthResponse.Str());
        return {TInstant::Zero(), Nothing(), false};
    }

    TMaybe<TString> user = GetHeader(webauthResponseHeaders, "X-Webauth-Login");
    if (user.Defined()) {
        LOG("Hello " << user);
    }
    TMaybe<TString> denialReason = GetHeader(webauthResponseHeaders, "Webauth-Denial-Reasons");
    if (denialReason.Defined()) {
        ERRORLOG("Not authorized: " << denialReason);
        return {TInstant::Now(), user, false};
    }

    return {TInstant::Now(), user, true};
}

void TMasterHttpRequest2::ReplyUnsafe(IOutputStream& out) {
    TMaybe<THttpRequestContext> context = THttpRequestContext::Cons(RD, RequestString);

    if (context.Empty()) {
        // UrlRoot isn't initialized - it's better not to use TMasterHttpRequest::ServeSimpleStatus
        NHttpUtil::ServeSimpleStatus(out, HTTP_BAD_REQUEST, "", "");
    }

    RequesterName = context->Requester;
    if (ReadOnly)
        RequesterName += " RO";

    UrlString = context->UrlString;
    Query = context->Query;
    Url = context->Url;
    UrlRoot = context->UrlRoot;

    if (Url.size() > 0 && Url[0] == "v1") {
        if (Url.size() == 2 && Url[1] == "data") {
            ServeDataVer1(out);
            return;
        }
        if (Url.size() == 1) {
            UrlString = "/v1/index.html";
        }
        ServeStaticFile(out, UrlString, mimetypeByExt(UrlString.data()));
    }
    else {
        Serve(out);
    }
}

void TMasterHttpRequest2::ServeStaticFile(IOutputStream& out, const TString& fileName, const TString& mime) {
    switch (mimeByStr(mime)) {
    case MIME_TEXT:
    case MIME_HTML:
    case MIME_XML:
    case MIME_CSS:
    case MIME_JAVASCRIPT:
        Output().EnableCompression(true);
        break;
    default:
        Output().EnableCompression(false);
    }

    time_t ifModifiedSince = 0;
    const TString *ims = RD.HeaderIn("If-Modified-Since");
    if (ims) {
        ParseHTTPDateTime(ims->c_str(), ifModifiedSince);
    }

    TFileStat st(TString("./") + fileName);
    if (!st.IsNull()) {
        if (st.MTime <= ifModifiedSince) {
            out << "HTTP/1.0 304 Not Modified\r\n";
            out << "\r\n";
            return;
        }
        else {
            out << "HTTP/1.0 200 OK\r\n";
            out << "Last-Modified: " << FormatHttpDate(st.MTime) << "\r\n";
            out << "Content-Type: " << mime << "\r\n";
            out << "\r\n";

            TUnbufferedFileInput in(TString("./") + fileName);
            TransferData(&in, &out);
            return;
        }
    }
    else {
    }

    ServeSimpleStatus(out, HTTP_NOT_FOUND);
}

void TMasterHttpRequest2::ServeDataVer1(IOutputStream& out) {

    TLockableHandle<TMasterGraph>::TReloadScriptReadLock graph(GetGraph());
    TLockableHandle<TWorkerPool>::TReloadScriptReadLock pool(GetPool());

    time_t reloadTimestamp = graph->GetTimestamp().Seconds();
    time_t modifiedSince = 0;
    const TString *ims = RD.HeaderIn("If-Modified-Since");
    if (ims) {
        ParseHTTPDateTime(ims->c_str(), modifiedSince);
    }

    if (reloadTimestamp <= modifiedSince) {
        out << "HTTP/1.0 304 Not Modified\r\n";
        out << "\r\n";
        return;
    }

    Output().EnableCompression(true);

    out << "HTTP/1.0 200 OK\r\n";
    out << "Last-Modified: " << FormatHttpDate(reloadTimestamp) << "\r\n";
    out << "Content-Type: application/json\r\n";
    out << "\r\n";

    unsigned number = 0;

    out << "\"master\":{";
    out << "\"version\":\"" << GetProgramSvnVersion() << '"';
    out << '}';

    out << ",\"workers\":{";
    for (const auto& worker : pool->GetWorkers()) {
        out << (number > 0 ? ",\"" : "\"") << worker->GetHost() << "\":{";
        out << "\"number\":" << number;
        out << ",\"group\":" << worker->GetGroupId();
        out << ",\"host\"" << '"' << worker->GetHost() << '"';
        out << ",\"port\"" << '"' << worker->GetPort() << '"';
        out << ",\"tasks\":0";
        out << '}';

        ++number;
    }
    out << '}';

    number = 0;
    out << ",\"targets\":{";
    for (const auto& target : graph->GetTargets()) {

        out << (number > 0 ? ",\"" : "\"") << target->Name << "\":{";
        out << "\"number\":" << number;
        out << ",\"type\":" << '"' << target->Type->GetName() << '"';
        out << ",\"name\":" << '"' << target->Name << '"';
        out << ",\"tasks\":" << target->Type->GetTaskCount() ;

        unsigned ii = 0;
        out << ",\"depends\":[";
        for (const auto& d : target->Depends) {
            out << (ii > 0 ? "," : "");
            d.IsCrossnode()
                ? out << "{\"name\":\"" << d.GetTarget()->Name << "\", \"crossnode\":1}"
                : out << "\"" << d.GetTarget()->Name << "\"";
            ++ii;
        }
        out << ']';

        ii = 0;
        out << ",\"follows\":[";
        for (const auto& f : target->Followers) {
            out << (ii > 0 ? "," : "");
            f.IsCrossnode()
                ? out << "{\"name\":\"" << f.GetTarget()->Name << "\", \"crossnode\":1}"
                : out << "\"" << f.GetTarget()->Name << "\"";
            ++ii;
        }
        out << ']';

        const TCronEntry* restartOnSuccess = target->GetRestartOnSuccessSchedule().Get();
        if (restartOnSuccess != nullptr) {
            out << ",\"restart_on_success\":" << '"' << *restartOnSuccess << '"' ;
        }

        const TCronEntry* retryOnFailure = target->GetRetryOnFailureSchedule().Get();
        if (retryOnFailure != nullptr) {
            out << ",\"retry_on_failure\":" << '"' << *retryOnFailure << '"' ;
        }

        if (target->GetRecipients() != nullptr) {
            const TRecipients* mailRecipients = target->GetRecipients()->GetMailRecipients();
            if (mailRecipients != nullptr) {
                unsigned ii = 0;
                out << ",\"mailto\":[";
                for (const auto& recip : *mailRecipients) {
                    out << (ii > 0 ? ",\"" : "\"") << recip << '"';
                    ++ii;
                }
                out << ']';
            }
        }

        out << '}';
        ++number;
    }
    out << '}';
}

TLockableHandle<TWorkerPool> TMasterHttpRequest::GetPool() {
    return reinterpret_cast<TMasterHttpServer*>(HttpServ())->GetPool();
}

TLockableHandle<TMasterGraph> TMasterHttpRequest::GetGraph() {
    return reinterpret_cast<TMasterHttpServer*>(HttpServ())->GetGraph();
}

void TMasterHttpRequest::ReplyUnsafe(IOutputStream& out) {
    TMaybe<THttpRequestContext> context = THttpRequestContext::Cons(RD, RequestString);

    if (context.Empty()) {
        // UrlRoot isn't initialized - it's better not to use TMasterHttpRequest::ServeSimpleStatus
        NHttpUtil::ServeSimpleStatus(out, HTTP_BAD_REQUEST, "", "");
    }

    RequesterName = context->Requester;
    if (ReadOnly)
        RequesterName += " RO";

    UrlString = context->UrlString;
    Query = context->Query;
    Url = context->Url;
    UrlRoot = context->UrlRoot;

    Serve(out);
}

TMaybe<TString> TMasterHttpRequest::GetCredetialsFromHeaders() {
    for (const std::pair<TString, TString>& header : ParsedHeaders) {
        if (AsciiEqualsIgnoreCase(header.first, "Authorization")) {
            return header.second;
        } else if (AsciiEqualsIgnoreCase(header.first, "Cookie")) {
            TStringBuf cookies(header.second);
            TStringBuf currentCookie;
            for (;cookies.NextTok("; ", currentCookie);) {
                if (currentCookie.StartsWith("Session_id=")) {
                    return TString{currentCookie};
                }
            }
        }
    }

    return Nothing();
}


TAtomicSharedPtr<TAuthorizationInfo> TMasterHttpRequest::RenewOldAuthenticationData(const TString& credentials, TAtomicSharedPtr<TAuthorizationInfo> cacheAuthRecord) {
    if (cacheAuthRecord->LastCheckTime + AUTHORIZATION_CACHE_TTL < TInstant::Now()) {
        AuthorizationCachePtr->Erase(credentials);
        cacheAuthRecord = AuthorizationCachePtr->Get(credentials);
    }

    return cacheAuthRecord;
}

TAtomicSharedPtr<TAuthorizationInfo> TMasterHttpRequest::GetAuthInfo(const TString& credentials) {
    TAtomicSharedPtr<TAuthorizationInfo> cacheAuthRecord = AuthorizationCachePtr->Get(credentials);
    RenewOldAuthenticationData(credentials, cacheAuthRecord);

    if (cacheAuthRecord.Get()->User.Defined()) {
        RequesterName = cacheAuthRecord.Get()->User.GetRef();
    }
    return cacheAuthRecord;
}

bool TMasterHttpRequest::CheckAuthorization(IOutputStream& out, TAtomicSharedPtr<TAuthorizationInfo> cacheAuthRecord) {
    if (MasterOptions.IdmRole) {
        Y_ENSURE(cacheAuthRecord.Get() != nullptr);
        if (cacheAuthRecord.Get()->User.Empty()) {
            ServeSimpleStatus(
                out,
                HTTP_UNAUTHORIZED,
                "Authentication failed: Check your cookie and token. Ask for support if it does not help.");
            return false;
        } else if (!cacheAuthRecord.Get()->Authorized) {
            ServeSimpleStatus(out, HTTP_FORBIDDEN, "User " + cacheAuthRecord.Get()->User.GetRef() + " is not authorized to use this master.");
            return false;
        }
    }

    return true;
}

void TMasterHttpRequest::Serve(IOutputStream& out) {
    if (MasterOptions.Authenticate) {
        TMaybe<TString> credentials = GetCredetialsFromHeaders();
        if (credentials.Empty()) {
            if (MasterOptions.IdmRole && !ReadOnly) {
                ServeSimpleStatus(out, HTTP_UNAUTHORIZED, "No Session_id cookie or OAuth token found.");
                return;
            }
        } else {
            TAtomicSharedPtr<TAuthorizationInfo> cacheAuthRecord = GetAuthInfo(credentials.GetRef());
            if (!ReadOnly && !CheckAuthorization(out, cacheAuthRecord)) {
                return;
            }
        }
    }

    AddMenuItem("Summary", "summary");
    AddMenuItem("Workers", "workers");
    AddMenuItem("Targets", "targets");
    AddMenuItem("Graph", "graph");
    AddMenuItem("Variables", "variables");

    GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_HTTP_REQUESTS_TOTAL);

    if (Url.size() == 0 || Url[0] == "summary") {
        ActivateMenuItem("Summary");
        ServeSummary(out);
    } else if (Url[0] == "style.css") {
        ServeStaticData(out, Url[0].data(), "text/css");
    } else if (Url[0] == "updater.js" ||
            Url[0] == "mouse.js" ||
            Url[0] == "hint.js" ||
            Url[0] == "menu.js" ||
            Url[0] == "sorter.js" ||
            Url[0] == "summary.js" ||
            Url[0] == "workers.js" ||
            Url[0] == "targets.js" ||
            Url[0] == "clusters.js" ||
            Url[0] == "graph.js" ||
            Url[0] == "variables.js" ||
            Url[0] == "monitoring.js" ||
            Url[0] == "jquery-2.1.1.js") {
        ServeStaticData(out, Url[0].data(), "text/javascript");
    } else if (Url[0] == "favicon.png") {
        ServeStaticData(out, Url[0].data(), "image/png");
    } else if (Url[0] == "env.js") {
        ServeEnvScript(out);
    } else if (Url[0] == "graph.svg") {
        ServeGraphImage(out);
    } else if (Url[0] == "graph.dot") {
        ServeGraphDot(out);
    } else if (Url[0] == "log") {
        if (Url.size() == 2) {
            LogLevel::Level() = FromString<ELogLevel>(Url[1]);
            ServeSimpleStatus(out, HTTP_OK);
        } else {
            ServeLog(out);
        }
    } else if (Url[0] == "ps") {
        ServePS(out);
    } else if (Url[0] == "throw") {
        ServeThrow(out);
    } else if (Url[0] == "status" && Url.size() == 1) { // Summary status
        ServeSummaryStatus(out);
    } else if (Url[0] == "workers") { // Workers
        ActivateMenuItem("Workers");
        ServeWorkers(out);
    } else if (Url[0] == "target" && Url.size() == 2) { // Workers for specified target
        AddActiveMenuItem(TString("Target: ") + Url[1], "target/" + Url[1]);
        ServeWorkers(out, Url[1]);
    } else if (Url[0] == "worker" && Url.size() == 3) { // Clusters
        AddMenuItem(TString("Worker: ") + Url[1], "worker/" + Url[1]);
        AddMenuItem(TString("Target: ") + Url[2], "target/" + Url[2]);
        AddActiveMenuItem(TString("Target ") + Url[2] + " on worker " + Url[1], "worker/" + Url[1] + "/" + Url[2]);
        AddMenuItem("&#8633;", "target/" + Url[2] + "/" + Url[1]);
        ServeClusters(out, Url[1], Url[2]);
    } else if (Url[0] == "status" && Url.size() == 2 && Url[1] == "workers") { // Workers status
        GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_HTTP_REQUESTS_STATUS_WORKERS);
        ServeWorkersStatus(out);
    } else if (Url[0] == "status" && Url.size() == 3 && Url[1] == "target") { // Workers status for specified target
        GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_HTTP_REQUESTS_STATUS_WORKERS);
        ServeWorkersStatus(out, Url[2]);
    } else if (Url[0] == "status" && Url.size() == 4 && Url[1] == "worker") { // Clusters status
        ServeClustersStatus(out, Url[2], Url[3]);
    } else if (Url[0] == "targets") { // Targets
        ActivateMenuItem("Targets");
        ServeTargets(out);
    } else if (Url[0] == "worker" && Url.size() == 2) { // Targets for specified worker
        AddActiveMenuItem(TString("Worker: ") + Url[1], "worker/" + Url[1]);
        ServeTargets(out, Url[1]);
    } else if (Url[0] == "target" && Url.size() == 3) { // Clusters
        AddMenuItem(TString("Target: ") + Url[1], "target/" + Url[1]);
        AddMenuItem(TString("Worker: ") + Url[2], "worker/" + Url[2]);
        AddActiveMenuItem(TString("Target ") + Url[1] + " on worker " + Url[2], "target/" + Url[1] + "/" + Url[2]);
        AddMenuItem("&#8633;", "worker/" + Url[2] + "/" + Url[1]);
        ServeClusters(out, Url[2], Url[1]);
    } else if (Url[0] == "status" && Url.size() == 2 && Url[1] == "targets") { // Targets status
        GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_HTTP_REQUESTS_STATUS_TARGETS);
        ServeTargetsStatus(out);
    } else if (Url[0] == "status" && Url.size() == 3 && Url[1] == "worker") { // Targets status for specified worker
        GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_HTTP_REQUESTS_STATUS_TARGETS);
        ServeTargetsStatus(out, Url[2]);
    } else if (Url[0] == "status" && Url.size() == 4 && Url[1] == "target") { // Clusters status
        ServeClustersStatus(out, Url[3], Url[2]);
    } else if (Url[0] == "graph" && Url.size() == 1) { // Graph
        ActivateMenuItem("Graph");
        ServeGraph(out);
    } else if (Url[0] == "graph" && Url.size() == 2) { // Graph for specified worker
        AddMenuItem(TString("Worker: ") + Url[1], "worker/" + Url[1]);
        AddActiveMenuItem(TString("Graph on worker ") + Url[1], "graph/" + Url[1]);
        ServeGraph(out, Url[1]);
    } else if (Url[0] == "status" && Url.size() == 2 && Url[1] == "graph") { // Graph status
        ServeGraphStatus(out);
    } else if (Url[0] == "status" && Url.size() == 3 && Url[1] == "graph") { // Graph status for specified worker
        ServeGraphStatus(out, Url[2]);
    } else if (Url[0] == "variables" && Url.size() == 1) { // Variables
        ActivateMenuItem("Variables");
        ServeVariables(out);
    } else if (Url[0] == "variables" && Url.size() == 2) { // Variables command
        if (!ReadOnly) {
            ServeVariablesCommand(out, Url[1]);
        } else {
            DEBUGLOG1(http, RequesterName << ": Requested variables command is forbidden in read-only");
            ServeSimpleStatus(out, HTTP_FORBIDDEN);
        }
    } else if (Url[0] == "monitoring" && Url.size() == 1) { // Monitoring
        ServeMonitoring(out);
    } else if (Url[0] == "status" && Url.size() == 2 && Url[1] == "monitoring") { // Monitoring status
        ServeMonitoringStatus(out);
    } else if (Url[0] == "command" && Url.size() == 2) { // Command
        GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_HTTP_REQUESTS_COMMAND);
        if (!ReadOnly) {
            ServeCommand(out, Url[1]);
        } else {
            DEBUGLOG1(http, RequesterName << ": Requested command is forbidden in read-only");
            ServeSimpleStatus(out, HTTP_FORBIDDEN);
        }
    } else if (Url[0] == "info" && Url.size() == 3 && Url[1] == "targets") {
        ServeTargetInfo(out, Url[2]);
    } else if (Url[0] == "info" && Url.size() == 4 && Url[1] == "worker") {
        ServeTargetInfo(out, Url[3], Url[2]);
    } else if (Url[0] == "info" && Url.size() == 5 && Url[1] == "target") {
        ServeClusterInfo(out, Url[3], Url[2], Url[4]);
    } else if (Url[0] == "info" && Url.size() == 5 && Url[1] == "worker") {
        ServeClusterInfo(out, Url[2], Url[3], Url[4]);
    } else if (Url[0] == "workers_text") {
        ServeWorkersText(out);
    } else if (Url[0] == "worker_text" && Url.size() == 2) {
        ServeTargetsText(out, Url[1]);
    } else if (Url[0] == "targets_text") {
        ServeTargetsText(out);
    } else if (Url[0] == "target_text" && Url.size() == 2) {
        ServeWorkersText(out, Url[1]);
    } else if (Url[0] == "target_text" && Url.size() == 3) { // Clusters
        ServeClustersText(out, Url[2], Url[1]);
    } else if (Url[0] == "targets_xml") {
        ServeTargetsXML(out);
    } else if (Url[0] == "targetstatus" && Url.size() == 1) {
        ServeTargetsStatusText(out);
    } else if (Url[0] == "targetstatus" && Url.size() == 2) {
        ServeTargetStatusText(out, Url[1]);
    } else if (Url[0] == "targetpathstatus" && Url.size() == 2) {
        ServeTargetPathStatusText(out, Url[1]);
    } else if (Url[0] == "cronstatus" && Url.size() == 2) {
        ServeTargetCronStatusText(out, Url[1]);
    } else if (Url[0] == "targetstatus" && Url.size() == 3) {
        ServeTargetStatusText(out, Url[1], Url[2]);
    } else if (Url[0] == "taskstatus" && Url.size() == 3) {
        ServeTaskStatusText(out, Url[1], FromString<ui32>(Url[2]));
    } else if (Url[0] == "taskpathstatus" && Url.size() == 3) {
        ServeTargetPathStatusText(out, Url[1], FromString<ui32>(Url[2]));
    } else if (Url[0] == "targettimes" && Url.size() == 2) {
        ServeTargetTimes(out, Url[1]);
    } else if (Url[0] == "targets_followers" && Url.size() == 2) {
        ServeTargetFollowers(out, Url[1]);
    } else if (Url[0] == "targets_dependencies" && Url.size() == 2) {
        ServeTargetDependencies(out, Url[1]);
    } else if (Url[0] == "targets_diamond" && Url.size() == 3) {
        ServeTargetsDiamond(out, Url[1], Url[2]);
    } else if (Url[0] == "target_toggle_restart" && Url.size() == 3) {
        if (!ReadOnly) {
            ServeTargetToggleRestartOnSuccessOrFailure(out, Url[1], Url[2]);
        } else {
            DEBUGLOG1(http, RequesterName << ": Requested toggle restart on success/failure is forbidden in read-only");
            ServeSimpleStatus(out, HTTP_FORBIDDEN);
        }
    } else if (Url[0] == "dump") {
        ServeDump(out);
    } else if (Url[0] == "proxy" && Url.size() >= 2) {
        TString workerUrl = "";

        if (Url.size() == 2)
            workerUrl = "/";
        else
            for (unsigned int i = 2; i < Url.size(); ++i)
                workerUrl += "/" + Url[i];

        ServeProxy(out, Url[1], workerUrl);
    } else if (Url[0] == "varstatus" && Url.size() == 2) {
        ServeVariableStatusText(out, Url[1]);
    } else if (Url[0] == "varstatus" && Url.size() == 1) {
        ServeVariableStatus(out);
    } else if (Url[0] == "counters_text") {
        ServeCountersText(out);
    } else if (Url[0] == "all_tasks_text") {
        ServeTasks(out, ETaskInfoFormat::TIF_TEXT);
    } else if (Url[0] == "tasks" && Url.size() < 3) {
        if (Url.size() == 1 || Url[1] == "json_short") {
            ServeTasks(out, ETaskInfoFormat::TIF_JSON_SHORT);
        } else if (Url[1] == "json") {
            ServeTasks(out, ETaskInfoFormat::TIF_JSON);
        } else if (Url[1] == "text") {
            ServeTasks(out, ETaskInfoFormat::TIF_TEXT);
        }
    } else {
        ServeSimpleStatus(out, HTTP_NOT_FOUND);
    }
}

void TMasterHttpRequest::ServeSimpleStatus(IOutputStream& out, HttpCodes code, const TString& annotation) {
    TString cmName = (MasterOptions.InstanceName.empty() ? ("at host &lt;" + MasterEnv.SelfHostname + "&gt;") : ("[" + MasterOptions.InstanceName + "]"));
    TString title = "ClusterMaster master " + cmName + ": error " + ToString<int>(static_cast<int>(code));
    NHttpUtil::ServeSimpleStatus(out, code, UrlRoot, title, annotation);
}

void TMasterHttpRequest::ServeRawData(IOutputStream& out, const char* data, const char* mime) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: " << mime << "\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";

    out << data;
}

void TMasterHttpRequest::ServeStaticData(IOutputStream& out, const char* name, const char* mime) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: " << mime << "\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n\r\n";

    GetStaticFile(TString("/") + name, out);
}

void TMasterHttpRequest::ServeEnvScript(IOutputStream& out) {
    PrintSimpleHttpHeader(out, "text/javascript");

    out << "Env={";
    out << "ReadOnly:" << (ReadOnly ? "true" : "false") << ",";
    out << "UpdaterDelay: " << MasterOptions.HTTPUpdaterDelay * 1000;
    out << "};";

    out << "\n\n";
    GetStaticFile("/env.js", out);
}

void TMasterHttpRequest::ServeGraphImage(IOutputStream& out) {
    TLockableHandle<TMasterGraph>::TReadLock gSnap(GetGraph());

    if (gSnap->GetGraphImage())
        ServeRawData(out, gSnap->GetGraphImage()->data(), "image/svg+xml");
    else
        ServeSimpleStatus(out, HTTP_SERVICE_UNAVAILABLE);
}

void TMasterHttpRequest::ServeGraphDot(IOutputStream& out) {
    TLockableHandle<TMasterGraph>::TReloadScriptReadLock gSnap(GetGraph());
    ServeRawData(out, gSnap->DumpGraphviz().data(), "text/x-dot");
}

void TMasterHttpRequest::FormatTaggedSubgraphs(IOutputStream& out, EPageTypes pt, const TTaggedSubgraphs& taggedSubgraphs) {
    if (!taggedSubgraphs.empty()) {
        // render tags bar
        TString activeTag = "__whole_graph__";
        if (Query.find("graphtag") != Query.end()) {
            activeTag = Query["graphtag"];
        }
        out << "<h1>Tagged subgraphs</h1>";
        out << "<ul class=\"tabs\">";
        out << "<li class=\"tab " << (activeTag == "__whole_graph__" ? "active_tab" : "") << "\">"
            << "<a href=\"" << UrlRoot << Url[0] << "\">&mdash;</a>"
            << "</li>";

        for (const auto& taggedSubgraphPair : taggedSubgraphs) {
            const TString& curTag = taggedSubgraphPair.first;
            out << "<li class=\"tab " << (curTag == activeTag ? "active_tab" : "") << "\">";
            switch (pt) {
                case PT_TARGETS:
                    out << "<a href=\"" << UrlRoot << "?graphtag=" << curTag << "\">" << curTag << "</a>";
                    break;
                case PT_GRAPH: {
                    TVector<TString> targetNames;
                    for (const auto& subgraph : taggedSubgraphPair.second) {
                        const TMasterTarget& target = **subgraph->GetTopoSortedTargets().rbegin();
                        targetNames.push_back(target.GetName());
                    }
                    out << "<a href=\"" << UrlRoot << "?graphtag=" << curTag << "&target=" << JoinSeq(",", targetNames) << "&walk=up&depth=-1\">" << curTag << "</a>";
                    break;
                }
                default:
                    Y_FAIL("Unsupported page type");
            }
            out << "</li>";
        }
        out << "</ul>";
    }
}


void TMasterHttpRequest::FormatMenu(IOutputStream& out) {
    out << "<ul class=\"menu\">";
    for (TVector<TMenuItem>::iterator i = Menu.begin(); i != Menu.end(); ++i) {
        if (i->Active)
            out << "<li class=\"active\">" << i->Name  << "</li>";
        else
            out << "<li><a href=\"" << UrlRoot << i->Url << "\">" << i->Name << "</a></li>";
    }
    out << "</ul>";
}

void TMasterHttpRequest::AddMenuItem(TString n, TString u) {
    Menu.push_back(TMenuItem(n, u, false));
}

void TMasterHttpRequest::AddActiveMenuItem(TString n, TString u) {
    Menu.push_back(TMenuItem(n, u, true));
}

void TMasterHttpRequest::ActivateMenuItem(TString n) {
    for (TVector<TMenuItem>::iterator i = Menu.begin(); i != Menu.end(); ++i)
        if (i->Name == n)
            i->Active = true;
}

void TMasterHttpRequest::FormatLegend(IOutputStream& out) {
    out << "<h1>Legend</h1>";
    out << "<div>";
    for (TStateRegistry::const_iterator i = TStateRegistry::begin(); i != TStateRegistry::end(); ++i)
        out << "<span class=\"target_" << i->SmallName << "\"> " << i->CapName << " </span>/";
    out << "<span style=\"font-weight: bold\"> Total </span>";
    out << "</div>";
}

void TMasterHttpRequest::FormatHeader(IOutputStream& out, EPageTypes pt) {
    out << "<html><head><title>ClusterMaster";
    if (MasterOptions.InstanceName.empty())
        out << " at host &lt;" << MasterEnv.SelfHostname << "&gt;:";
    else
        out << " [" << MasterOptions.InstanceName << "]";
    out << "</title>";

    out << "<meta charset=\"UTF-8\">";
    out << "<link rel=\"icon\" href=\"" << UrlRoot << "favicon.png\" type=\"image/png\"/ >";
    out << "<link href=\"" << UrlRoot << "style.css\" rel=\"stylesheet\" type=\"text/css\" />";
    FormatScript(out, "jquery-2.1.1.js");
    FormatScript(out, "env.js");
    FormatScript(out, "updater.js");

    if (pt == PT_WORKERS || pt == PT_TARGETS || pt == PT_CLUSTERS || pt == PT_GRAPH) {
        FormatScript(out, "mouse.js");
        FormatScript(out, "menu.js");
        FormatScript(out, "hint.js");
        FormatScript(out, "sorter.js");
    }

    out << "<script type=\"text/javascript\">PageEnv={\"UrlRoot\":\"" + UrlRoot + "\"}</script>";

    switch (pt) {
        case PT_SUMMARY: FormatScript(out, "summary.js"); break;
        case PT_WORKERS: FormatScript(out, "workers.js"); break;
        case PT_TARGETS: FormatScript(out, "targets.js"); break;
        case PT_CLUSTERS: FormatScript(out, "clusters.js"); break;
        case PT_GRAPH: FormatScript(out, "graph.js"); break;
        case PT_VARIABLES: FormatScript(out, "variables.js"); break;
        case PT_MONITORING: FormatScript(out, "monitoring.js"); break;
    }

    out << "</head><body>";

    if (pt != PT_MONITORING) {
        out << "<div id=\"notification\"></div>";

        FormatMenu(out);
    }
}

void TMasterHttpRequest::FormatScript(IOutputStream& out, TString fileName) {
    out << "<script src=\"" << UrlRoot << fileName << "\" type=\"text/javascript\"></script>";
}

void TMasterHttpRequest::FormatFooter(IOutputStream& out, EPageTypes pt) {
    if (pt != PT_SUMMARY && pt != PT_MONITORING)
        FormatLegend(out);

    out << "</body></html>";
}

TString MimeTypeByFormat(TMasterHttpRequest::ETaskInfoFormat format) {
    switch (format) {
        case TMasterHttpRequest::ETaskInfoFormat::TIF_TEXT:
            return "text/plain";
        case TMasterHttpRequest::ETaskInfoFormat::TIF_JSON:
            // [[fallthrough]];
        case TMasterHttpRequest::ETaskInfoFormat::TIF_JSON_SHORT:
            return "application/json";
    }
    return "text/plain";
}
