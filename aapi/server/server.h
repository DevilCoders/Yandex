#pragma once

#include "config.h"
#include "ping.h"

#include <aapi/lib/common/walk.h>
#include <aapi/lib/proto/vcs.grpc.pb.h>
#include <aapi/lib/store/disc_store.h>
#include <aapi/lib/store/ram_store.h>
#include <aapi/lib/yt/async_lookups.h>
#include <aapi/lib/yt/async_lookups2.h>

#include <mapreduce/yt/interface/client.h>
#include <yweb/robot/js/lib/monitor/counter.h>
#include <yweb/robot/js/lib/monitor/page.h>

#include <contrib/libs/grpc/include/grpc++/grpc++.h>
#include <contrib/libs/grpc/include/grpc++/server.h>
#include <contrib/libs/grpc/include/grpc++/server_context.h>

#include <library/cpp/threading/future/future.h>

#include <util/folder/path.h>
#include <util/generic/deque.h>
#include <util/random/shuffle.h>
#include <util/system/mutex.h>
#include <util/thread/factory.h>
#include <util/thread/pool.h>

namespace NAapi {

using ::NAapi::NStore::TDiscStore;
using ::NAapi::NStore::TCompoundRamStore;
using ::NAapi::NStore::TRamStore;
using ::NAapi::NStore::TCache;
using ::NAapi::NStore::TStatus;

static const TString RPS_SENSOR("rps");

static const ui64 MAX_RAM_OBJECT_SIZE = 262144;

class IWalkReceiver : public TThrRefBase {
public:
    virtual ~IWalkReceiver() = default;

    virtual bool Push(const TDirectories& dirs) = 0;

    virtual void Finish(const grpc::Status& status) = 0;
};

class TVcsServer : public IThreadFactory::IThreadAble {
    static const TVector<ui32> RequestTimeIntervals;
    static const TVector<ui32> RequestKeysCountIntervals;

    /// Общие счётчики сервера.
    struct TCounters : public NMonitor::TCounterSource {
        /// Количество запросов Objects2.
        NMonitor::TDerivCounter NRequestObjects2;
        /// Количество запросов Objects3.
        NMonitor::TDerivCounter NRequestObjects3;
        /// Количество запросов Push.
        NMonitor::TDerivCounter NRequestPush;
        /// Количество запросов NotifyNewObjects
        NMonitor::TDerivCounter NRequestsNotifyNewObjects;
        /// Количество запросов SvnHead.
        NMonitor::TDerivCounter NRequestSvnHead;
        /// Количество запросов HgId.
        NMonitor::TDerivCounter NRequestHgId;
        /// Количество запросов Walk.
        NMonitor::TDerivCounter NRequestWalk;

        /// Текущее количество сессий Objects2.
        NMonitor::TCounter      NSessionObjects2;
        /// Текущее количество сессий Objects3.
        NMonitor::TCounter      NSessionObjects3;
        /// Текущее количество сессий Walk.
        NMonitor::TCounter      NSessionWalk;

        /// Текущее количество объектов TRead для Objects3
        NMonitor::TCounter NObjects3ReadObjects;

        /// Количество успешных запросов из кэша на диске.
        NMonitor::TDerivCounter NCacheDiskHits;
        /// Количество успешных запросов из кэша в памяти.
        NMonitor::TDerivCounter NCacheMemoryHits;
        /// Количество запросов, для которых в кэше не было данных.
        NMonitor::TDerivCounter NCacheMisses;
        /// Количество объектов, помещённых в кэш.
        NMonitor::TDerivCounter NCachePuts;

        /// Текущее количество выполняющихся запросов к YT.
        NMonitor::TCounter      NYTInflight;
        /// Количество запросов LookupRows к YT.
        NMonitor::TDerivCounter NYTLookupCalls;

        void QueryCounters(NMonitor::TCounterTable* ct) const override;
    };

    struct TDistributionCounters : public NMonitor::TCounterSource {
        explicit TDistributionCounters(const TVector<ui32>& grid);

        void AddValue(ui64 value, ui64 weight = 1);

        void QueryCounters(NMonitor::TCounterTable* ct) const override;

        virtual ~TDistributionCounters() = default;

        virtual TString MakeGridPointName(ui64 i) const = 0;

        const TVector<ui32>& Grid;
        TVector<NMonitor::TDerivCounter> Distribution;
    };

    //! Счётчики по распределению времён ответов YT.
    struct TLatencyCounters : public TDistributionCounters {
        TLatencyCounters();
        TString MakeGridPointName(ui64 i) const override;
    };

    //! Счётчики по распределению времён ответов YT, нормированные на количество ключей в запросе.
    struct TNormalizedLatencyCounters : public TDistributionCounters {
        TNormalizedLatencyCounters();
        TString MakeGridPointName(ui64 i) const override;
    };

    //! Счётчики по распределению количеств ключей в запросах к YT.
    struct TRequestKeysCounters : public TDistributionCounters {
        TRequestKeysCounters();
        TString MakeGridPointName(ui64 i) const override;
    };

    using TSvnHeadCallback = std::function<void(ui64 revision)>;

public:
    TVcsServer(const TConfig& config);
    ~TVcsServer();

    NThreading::TFuture<ui64> AsyncGetSvnHead();

    NThreading::TFuture<TString> AsyncGetHgId(const TString& name);

    void AsyncPrefetchObjects(const TVector<TString>& hashes);

    void AsyncWalk(const TString& root, TIntrusivePtr<IWalkReceiver> recv);

    void ScheduleGetObject(const TString& hash, TGetObjectCallback cb);

    TString HgId(const TString& name);

    void JoinThreads();

private:
    void InitilaizeServer(const TString& address);

    void WaitObjects2();
    void WaitObjects3();
    void WaitWalk();
    void WaitSvnHead();
    void WaitHgId();
    void WaitPush();
    void WaitNotifyNewObjects();

    void DoExecute() override;

public:
    TVector<NYT::TNode> LookupRows(const TVector<NYT::TNode>& keys);

    void MaybeWarmupCache(const TVector<TString>& paths);

    bool Lookup(const TString& key, TString& blob);
    void Put(const TString& key, const TString& blob);

private:
    class TSvnHeadRequest;
    class THgIdRequest;
    class TWalkRequest;
    class TObject2Request;
    class TObject3Request;
    class TPushRequest;
    class TNotifyNewObjectsRequest;

    using IThreadRef = TAutoPtr<IThreadFactory::IThread>;

    NMonitor::TBasicPage CountersPage_;
    NMonitor::THttpService MonitorService_;
    TCounters Counters_;
    TLatencyCounters LatencyCounters_;
    TNormalizedLatencyCounters NormalizedLatencyCounters_;
    TRequestKeysCounters KeysCounters_;

    TDiscStore DiscStore;
    TCompoundRamStore RamStore;
    NYT::IClientPtr Client;
    const TString Table;
    const TString SvnHeadPath;
    TAutoPtr<IThreadPool> Pool;
    TAutoPtr<IThreadPool> FetchPool;
    TAsyncLookups2<TVcsServer> AsyncLookups;

    TMutex Lock_;
    TVector<NThreading::TPromise<ui64>> RevisionWaitList_;
    TAtomic LastKnownSvnHead_;

    TMutex HgLock;
    THashMap<TString, TVector<NThreading::TPromise<TString> > > HgIdWaitLists;

    NVcs::Vcs::AsyncService Service_;
    std::unique_ptr<grpc::Server> Server_;
    std::unique_ptr<grpc::ServerCompletionQueue> CQ_;
    TVector<IThreadRef> Ts_;

    TPingThread Ping_;

    const TString HgBinaryPath;
    const TString HgServer;
    const TString HgUser;
    const TString HgKey;
};

}  // namespace NAapi
