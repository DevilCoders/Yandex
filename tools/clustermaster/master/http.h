#pragma once

#include "master_target_graph.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/http_logging.h>
#include <tools/clustermaster/common/lockablehandle.h>
#include <tools/clustermaster/communism/util/http_util.h>

#include <library/cpp/cache/thread_safe_cache.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/xml/encode/encodexml.h>

#include <util/generic/buffer.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/stream/buffer.h>
#include <util/string/escape.h>
#include <util/string/vector.h>

class TWorkerPool;
class TMasterGraph;

struct TStateFilter;

struct TAuthorizationInfo {
    TInstant LastCheckTime;
    TMaybe<TString> User;
    bool Authorized;
};

using TAuthorizationCache = TThreadSafeCache<TString, TAuthorizationInfo, const TString&>;
using TAuthorizationCachePtr = TSharedPtr<TAuthorizationCache, TAtomicCounter>;

class TAuthorizationCallbacks: public TThreadSafeCache<TString, TAuthorizationInfo, const TString&>::ICallbacks {
public:
    TString GetKey(const TString& key) const override;
    TAuthorizationInfo* CreateObject(const TString& key) const override;
    TAuthorizationInfo GetUserWithAuthorization(const TString& cookie, const TString& token) const;
};

class TAuthorizationException : public yexception {};

class TMasterHttpRequest: public THttpClientRequestForHuman<TClustermasterHttpRequestLogger>, TNonCopyable {
public:
    TMasterHttpRequest(bool ro, TAuthorizationCachePtr authorizationCachePtr)
        : ReadOnly(ro)
        , AuthorizationCachePtr(std::move(authorizationCachePtr))
    {
    }

    enum class ETaskInfoFormat {
        TIF_JSON,
        TIF_JSON_SHORT,
        TIF_TEXT,
    };
private:
    enum EPageTypes {
        PT_SUMMARY,
        PT_WORKERS,
        PT_TARGETS,
        PT_CLUSTERS,
        PT_GRAPH,
        PT_VARIABLES,
        PT_MONITORING,
    };
    static constexpr TDuration AUTHORIZATION_CACHE_TTL = TDuration::Minutes(10);

protected:
    void ReplyUnsafe(IOutputStream&) override;
    void ServeSimpleStatus(IOutputStream&, HttpCodes code, const TString& annotation = "");

    TLockableHandle<TWorkerPool> GetPool();
    TLockableHandle<TMasterGraph> GetGraph();

    void Serve(IOutputStream&);
    TMaybe<TString> GetUserWithAuthorization(const TString& cookie, const TString& token);

private:
    void ServeRawData(IOutputStream&, const char* data, const char* mime);
    void ServeStaticData(IOutputStream&, const char* name, const char* mime);
    void ServeEnvScript(IOutputStream&);
    void ServeGraphImage(IOutputStream&);
    void ServeGraphDot(IOutputStream&);

    void ServeSummary(IOutputStream&);
    void ServeWorkers(IOutputStream&, const TString& targetName = "");
    void ServeTargets(IOutputStream&, const TString& workerHost = "");
    void ServeTargetCronStatusText(IOutputStream&, const TString& targetName);
    void ServeClusters(IOutputStream&, const TString& workerHost, const TString& targetName);
    void ServeClustersText(IOutputStream&, const TString& workerHost, const TString& targetName);
    void ServeGraph(IOutputStream&, const TString& workerHost = "");
    void ServeMonitoring(IOutputStream&);

    void ServeSummaryStatus(IOutputStream&);
    void ServeWorkersStatus(IOutputStream&, const TString& targetName = "");
    void ServeTargetsStatus(IOutputStream&, const TString& workerHost = "");
    void ServeClustersStatus(IOutputStream&, const TString& workerHost, const TString& targetName);
    void ServeGraphStatus(IOutputStream&, const TString& workerHost = "");
    void ServeMonitoringStatus(IOutputStream&);

    void ServeTargetInfo(IOutputStream&, const TString& targetName, TString workerHost = "");
    void ServeWorkerInfo(IOutputStream&, const TString& workerHost, TString targetName = "");
    void ServeClusterInfo(IOutputStream&, const TString& workerHost, const TString& targetName, const TString& taskNumber);

    void ServeVariables(IOutputStream&);
    void ServeVariableStatusText(IOutputStream&, const TString& name = "");
    void ServeVariableStatus(IOutputStream&);

    void CheckConditionIfNeeded(IOutputStream&, ui32 flags, TMasterGraph::TTargetsList::const_iterator target,
            const IWorkerPoolVariables* pool);
    void ServeCommand(IOutputStream&, const TString& cmd);
    void ServeVariablesCommand(IOutputStream&, const TString& cmd);

    void ServeLog(IOutputStream&);
    void ServePS(IOutputStream&);
    void ServeProxy(IOutputStream&, const TString& workerHost, const TString& url);
    void ServeThrow(IOutputStream&);

    void ServeTargetsText(IOutputStream&, const TString& workerHost = "");
    void ServeWorkersText(IOutputStream&, const TString& targetName = "");

    void ServeTargetsXML(IOutputStream&);

    void ServeTargetsDescriptions(IOutputStream&);
    void ServeTargetsStatusText(IOutputStream&);
    void ServeTargetStatusText(IOutputStream&, const TString& name);
    void ServeTargetStatusText(IOutputStream&, const TString& name, const TString& workerName);
    void ServeTargetPathStatusText(IOutputStream&, const TString& name, TMaybe<ui32> taskId = Nothing());
    void ServeTaskStatusText(IOutputStream&, const TString& name, ui32 taskId);
    void ServeTargetTimes(IOutputStream&, const TString& name);

    void ServeTargetList(IOutputStream&, const TVector<TString>& targets);
    void ServeTargetFollowers(IOutputStream&, const TString& target);
    void ServeTargetDependencies(IOutputStream&, const TString& target);
    void ServeTargetsDiamond(IOutputStream&, const TString& target1, const TString& target2);

    void ServeTargetToggleRestartOnSuccessOrFailure(IOutputStream&, const TString& targetName, const TString& successOrFailure);

    void ServeDump(IOutputStream&);

    void ServeCountersText(IOutputStream&);

    static void ServeTasksOneTarget(IOutputStream& out, const TMasterGraph::TTarget& target, const TWorker* worker,
        const TStateFilter& stateFilter, ETaskInfoFormat format = ETaskInfoFormat::TIF_TEXT);
    void ServeTasks(IOutputStream& out, ETaskInfoFormat format);

    void FormatLegend(IOutputStream& out);
    void FormatHeader(IOutputStream& out, EPageTypes pt);
    void FormatFooter(IOutputStream& out, EPageTypes pt);
    void FormatScript(IOutputStream& out, TString fileName);

    template<typename TTargetsList>
        void FormatTargetsList(IOutputStream& out, const TTargetsList& targets, const TWorker* worker, TStateFilter& stateFilter, size_t& number);

    void FormatTaggedSubgraphs(IOutputStream& out, EPageTypes pt, const TTaggedSubgraphs& taggedSubgraphs);

    void FormatMenu(IOutputStream& out);

    void AddMenuItem(TString n, TString u);
    void AddActiveMenuItem(TString n, TString u);
    void ActivateMenuItem(TString n);
    TMaybe<TString> GetCredetialsFromHeaders();
    TAtomicSharedPtr<TAuthorizationInfo> RenewOldAuthenticationData(const TString& credentials, TAtomicSharedPtr<TAuthorizationInfo> cacheAuthRecord);
    TAtomicSharedPtr<TAuthorizationInfo> GetAuthInfo(const TString& credentials);
    bool CheckAuthorization(IOutputStream& out, TAtomicSharedPtr<TAuthorizationInfo> cacheAuthRecord);

private:
    struct TMenuItem {
        TString Name;
        TString Url;
        bool Active;

        TMenuItem(TString n, TString u, bool a)
            : Name(n)
            , Url(u)
            , Active(a)
        {
        }
    };

    typedef THashMap<TString, TString> TQueryMap;
    typedef TVector<TString> TUrlList;

protected:
    TString RequesterName;
    TString UrlString;
    TString UrlRoot;
    TQueryMap Query;
    TUrlList Url;
    TVector<TMenuItem> Menu;
    const bool ReadOnly;
    TAuthorizationCachePtr AuthorizationCachePtr;
};

class TMasterHttpRequest2: public TMasterHttpRequest {
public:
    TMasterHttpRequest2(bool ro, TAuthorizationCachePtr authorizationCachePtr)
        : TMasterHttpRequest(ro, std::move(authorizationCachePtr))
    {
    }
    void ReplyUnsafe(IOutputStream&) override;

    void ServeStaticFile(IOutputStream& out, const TString& fileName, const TString& mime);

    void ServeDataVer1(IOutputStream& out);
    void ServeDiffVer1(IOutputStream& out);

};

class TMasterHttpServer: public THttpServer, THttpServer::ICallBack {
private:
    static constexpr ui8 CACHE_SIZE = 100;
public:
    TMasterHttpServer(TLockableHandle<TWorkerPool> wp, TLockableHandle<TMasterGraph> tg, THttpServer::TOptions options, bool ro)
        : THttpServer(this, options)
        , WorkerPool(wp)
        , TargetGraph(tg)
        , ReadOnly(ro)
        , AuthorizationCallbacks()
        , AuthorizationCachePtr(new TAuthorizationCache(AuthorizationCallbacks, CACHE_SIZE))
    {
    }

    TLockableHandle<TWorkerPool> GetPool() {
        return WorkerPool;
    }

    TLockableHandle<TMasterGraph> GetGraph() {
        return TargetGraph;
    }

protected:
    TClientRequest* CreateClient() override {
        return new TMasterHttpRequest2(ReadOnly, AuthorizationCachePtr);
    }

protected:
    TLockableHandle<TWorkerPool> WorkerPool;
    TLockableHandle<TMasterGraph> TargetGraph;

    const bool ReadOnly;
    TAuthorizationCallbacks AuthorizationCallbacks;
    TAuthorizationCachePtr AuthorizationCachePtr;
};

TString MimeTypeByFormat(TMasterHttpRequest::ETaskInfoFormat format);
void FormatLogLinks(ui8 numberOfPreviousTargetLogs, const TString& UrlRoot, const TString& workerHost, const TString& targetName, int taskNumber);
