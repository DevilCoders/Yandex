#include "event.h"
#include "notify.h"
#include "objects2.h"
#include "objects3.h"
#include "push.h"
#include "server.h"
#include "svn_head.h"
#include "hg_id.h"
#include "tracers.h"
#include "walk.h"
#include "warmup.h"

#include <aapi/lib/common/async_reader.h>
#include <aapi/lib/common/async_transfer.h>
#include <aapi/lib/common/async_writer.h>
#include <aapi/lib/sensors/sensors.h>
#include <aapi/lib/trace/events.ev.pb.h>
#include <aapi/lib/trace/trace.h>

#include <library/cpp/blockcodecs/stream.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/system/hp_timer.h>
#include <util/thread/factory.h>
#include <util/random/random.h>
#include <util/stream/pipe.h>

using namespace NThreading;

namespace NAapi {
namespace {

struct TWalkElement {
    TWalkElement(TVector<TString> path, TString key, TString data, const grpc::Status& st = grpc::Status::OK)
        : Path(std::move(path))
        , Key(std::move(key))
        , Data(data)
        , St(st)
    {
    }

    TVector<TString> Path;
    TString Key;
    TString Data;
    grpc::Status St;
};

ui64 TreeLookupWeight() {
    return static_cast<ui64>(RandomNumber<ui64>() % 7 == 0);
}

}  // namespace

const TVector<ui32> TVcsServer::RequestTimeIntervals = {
    1, 10, 50, 100, 250, 375, 500, 625, 750, 875, 1000, 2500, 5000, 10000, 100000
};

const TVector<ui32> TVcsServer::RequestKeysCountIntervals = {
    1, 2, 3, 4, 5, 10, 25, 50, 100, 250, 500, 1000, 5000, 10000, 25000
};

void TVcsServer::TCounters::QueryCounters(NMonitor::TCounterTable* ct) const {
    ct->insert(MAKE_COUNTER_PAIR(NRequestObjects2));
    ct->insert(MAKE_COUNTER_PAIR(NRequestObjects3));
    ct->insert(MAKE_COUNTER_PAIR(NRequestPush));
    ct->insert(MAKE_COUNTER_PAIR(NRequestsNotifyNewObjects));
    ct->insert(MAKE_COUNTER_PAIR(NRequestSvnHead));
    ct->insert(MAKE_COUNTER_PAIR(NRequestHgId));
    ct->insert(MAKE_COUNTER_PAIR(NRequestWalk));

    ct->insert(MAKE_COUNTER_PAIR(NSessionObjects2));
    ct->insert(MAKE_COUNTER_PAIR(NSessionObjects3));
    ct->insert(MAKE_COUNTER_PAIR(NSessionWalk));

    ct->insert(MAKE_COUNTER_PAIR(NObjects3ReadObjects));

    ct->insert(MAKE_COUNTER_PAIR(NCacheDiskHits));
    ct->insert(MAKE_COUNTER_PAIR(NCacheMemoryHits));
    ct->insert(MAKE_COUNTER_PAIR(NCacheMisses));
    ct->insert(MAKE_COUNTER_PAIR(NCachePuts));

    ct->insert(MAKE_COUNTER_PAIR(NYTInflight));
    ct->insert(MAKE_COUNTER_PAIR(NYTLookupCalls));
}

TVcsServer::TDistributionCounters::TDistributionCounters(const TVector<ui32>& grid)
    : Grid(grid)
    , Distribution(grid.size())
{
}

void TVcsServer::TDistributionCounters::AddValue(ui64 value, ui64 weight) {
    auto bi = std::lower_bound(Grid.begin(), Grid.end(), value);
    if (bi == Grid.end()) {
        Distribution.back() += weight;
    } else {
        Distribution[bi - Grid.begin()] += weight;
    }
}

void TVcsServer::TDistributionCounters::QueryCounters(NMonitor::TCounterTable* ct) const {
    for (size_t i = 0; i < Distribution.size(); ++i) {
        ct->insert(
            MAKE_NAMED_COUNTER_PAIR(MakeGridPointName(i), Distribution[i])
        );
    }
}

TVcsServer::TLatencyCounters::TLatencyCounters()
    : TDistributionCounters(RequestTimeIntervals)
{
}

TString TVcsServer::TLatencyCounters::MakeGridPointName(ui64 i) const {
    return Sprintf("NYTLatencyLess_%ims", Grid[i]);
}

TVcsServer::TNormalizedLatencyCounters::TNormalizedLatencyCounters()
    : TDistributionCounters(RequestTimeIntervals)
{
}

TString TVcsServer::TNormalizedLatencyCounters::MakeGridPointName(ui64 i) const {
    return Sprintf("NYTNormalizedLatencyLess_%ims", Grid[i]);
}

TVcsServer::TRequestKeysCounters::TRequestKeysCounters()
    : TDistributionCounters(RequestKeysCountIntervals)
{
}

TString TVcsServer::TRequestKeysCounters::MakeGridPointName(ui64 i) const {
    return Sprintf("NYTKeysCountLess_%i", Grid[i]);
}

TVcsServer::TVcsServer(const TConfig& config)
    : MonitorService_(NMonitor::SimpleConfig(config.MonitorPort))
    , DiscStore(config.DiscStorePath)
    , RamStore(config.RamStoreSize)
    , Client(NYT::CreateClient(config.YtProxy, NYT::TCreateClientOptions().Token(config.YtToken)))
    , Table(config.YtTable)
    , SvnHeadPath(config.YtSvnHead)
    , Pool(CreateThreadPool(config.InnerPoolSize))
    , FetchPool(CreateThreadPool(8))
    , AsyncLookups(FetchPool.Get(), 4, 64, this)
    , LastKnownSvnHead_(0)
    , Ping_(config.Host, 60000)
    , HgBinaryPath(config.HgBinaryPath)
    , HgServer(config.HgServer)
    , HgUser(config.HgUser)
    , HgKey(config.HgKey)
{
    InitSensor(RPS_SENSOR, 0, true);
    InitilaizeServer(Join(":", "[::]", config.AsyncPort));
    //
    // Register counters
    //
    CountersPage_.AddToHttpService(&MonitorService_);
    CountersPage_.RegisterSource(&Counters_, "Server");
    CountersPage_.RegisterSource(&LatencyCounters_, "Latency");
    CountersPage_.RegisterSource(&NormalizedLatencyCounters_, "NormalizedLatency");
    CountersPage_.RegisterSource(&KeysCounters_, "LookupRequestKeysCount");
    MonitorService_.Start();

    for (size_t i = 0; i < config.AsyncServerThreads; ++i) {
        Ts_.push_back(SystemThreadFactory()->Run(this));
    }

    MaybeWarmupCache(config.WarmupPaths);
}

void TVcsServer::JoinThreads() {
    for (auto ti = Ts_.begin(); ti != Ts_.end(); ++ti) {
        (*ti)->Join();
    }
}

TVcsServer::~TVcsServer() {
    Server_->Shutdown();
    // Always shutdown the completion queue after the server.
    CQ_->Shutdown();

    JoinThreads();
}

TVector<NYT::TNode> TVcsServer::LookupRows(const TVector<NYT::TNode>& keys) {
    try {
        NYT::TLookupRowsOptions options;
        options.KeepMissingRows(true);
        options.Columns({"type", "data"});

        Counters_.NYTLookupCalls.Inc();
        Counters_.NYTInflight.Inc();

        ui64 start = GetCycleCount();
        TVector<NYT::TNode> result = Client->LookupRows(Table, keys, options);
        ui64 finish = GetCycleCount();

        Counters_.NYTInflight.Dec();

        {
            ui64 ms = (1000 * (finish - start)) / NHPTimer::GetCyclesPerSecond();
            LatencyCounters_.AddValue(ms, 1);
            NormalizedLatencyCounters_.AddValue(ms / keys.size(), keys.size());
            KeysCounters_.AddValue(keys.size(), 1);
        }

        return result;
    } catch (...) {
        Counters_.NYTInflight.Dec();
        throw;
    }

    return TVector<NYT::TNode>();
}

void TVcsServer::MaybeWarmupCache(const TVector<TString>& paths) {
    if (paths.empty()) {
        return;
    }

    AsyncGetSvnHead()
        .Subscribe([this, paths] (TFuture<ui64> result) {
            if (result.GetValue() == 0) {
                return;
            }

            Pool->SafeAddFunc([this, paths, revision = result.GetValue()] ()
                {
                    TIntrusivePtr<TWarmupProcessor> proc(new TWarmupProcessor(paths, this));
                    proc->Walk(revision);
                }
            );
        }
    );
}

bool TVcsServer::Lookup(const TString& key, TString& blob) {
    if (RamStore.Get(key, blob)) {
        Counters_.NCacheMemoryHits.Inc();
        return true;
    } else if (DiscStore.Get(key, blob).IsOk()) {
        if (Y_LIKELY(blob.size() <= MAX_RAM_OBJECT_SIZE)) {
            RamStore.Put(key, blob);
        }
        Counters_.NCacheDiskHits.Inc();
        return true;
    }
    Counters_.NCacheMisses.Inc();
    return false;
}

void TVcsServer::Put(const TString& key, const TString& blob) {
    Counters_.NCachePuts.Inc();

    if (Y_LIKELY(blob.size() <= MAX_RAM_OBJECT_SIZE)) {
        RamStore.Put(key, blob);
    }
    DiscStore.Put(key, blob);
}

///////////////////////////////////////////////////////////////////////////////
// ASYNC SERVICE

void TVcsServer::InitilaizeServer(const TString& address) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&Service_);
    builder.SetMaxReceiveMessageSize(32 * 1024 * 1024);
    CQ_ = builder.AddCompletionQueue();

    Server_ = builder.BuildAndStart();

    if (!Server_) {
        ythrow yexception() << "can't start server";
    }
}

void TVcsServer::AsyncPrefetchObjects(const TVector<TString>& hashes) {
    FetchPool->SafeAddFunc([this, hashes] ()
        {
            TString blob;
            for (const auto& h: hashes) {
                if (!Lookup(h, blob)) {
                    AsyncLookups.ScheduleLookup(h);
                }
            }
        });
}

void TVcsServer::AsyncWalk(const TString& root, TIntrusivePtr<IWalkReceiver> recv) {
    Pool->SafeAddFunc([this, root, recv] ()
        {
            TWalkTracer tracer(HexEncode(root));
            const size_t maxReturnDirs = 2500;
            TAtomicSharedPtr<NThreading::TBlockingQueue<TWalkElement>> queue(new NThreading::TBlockingQueue<TWalkElement>(0));

            TVector<TString> startPath(1, TString("."));
            TString startData;
            if (Lookup(root, startData)) {
                Y_ENSURE(queue->Push(TWalkElement(std::move(startPath), root, std::move(startData))));
                tracer.ObjectInCache();
            } else {
                AsyncLookups.ScheduleLookup(root, 0, [queue, startPath = std::move(startPath)](const TString& h, const TString& d, const grpc::Status& st) {
                    Y_ENSURE(queue->Push(TWalkElement(std::move(startPath), h, d, st)));
                });
            }
            ui64 queueSize = 1;

            TDirectories directories;
            TString data;
            TEntry dep;

            while (queueSize) {
                TWalkElement top = *queue->Pop();
                --queueSize;

                if (top.St.error_code() != grpc::StatusCode::OK) {
                    recv->Finish(top.St);
                }

                TDirectory* dir = directories.AddDirectories();
                dir->SetHash(top.Key);
                dir->SetBlob(top.Data);
                dir->SetPath(JoinRange("/", top.Path.cbegin(), top.Path.cend()));
                tracer.ObjectSent();

                if (directories.DirectoriesSize() >= maxReturnDirs) {
                    if (!recv->Push(directories)) {
                        recv->Finish(grpc::Status::CANCELLED);
                        return;
                    }
                    directories.Clear();
                }

                TEntriesIter depsIter(top.Data.data(), TEntriesIter::EIterMode::EIM_DIRS);

                while (depsIter.Next(dep)) {
                    if (Lookup(dep.Hash, data)) {
                        TWalkElement el(top.Path, std::move(dep.Hash), std::move(data));
                        el.Path.emplace_back(std::move(dep.Name));
                        Y_ENSURE(queue->Push(std::move(el)));
                        tracer.ObjectInCache();
                    } else {
                        AsyncLookups.ScheduleLookup(dep.Hash, TreeLookupWeight(), [queue, p = top.Path, n = dep.Name](const TString& h, const TString& d, const grpc::Status& st) {
                            TWalkElement el(p, h, d, st);
                            el.Path.emplace_back(n);
                            Y_ENSURE(queue->Push(std::move(el)));
                        });
                    }
                    ++queueSize;
                }
            }

            if (directories.DirectoriesSize()) {
                if (!recv->Push(directories)) {
                    recv->Finish(grpc::Status::CANCELLED);
                    return;
                }
            }

            tracer.Ok();
            recv->Finish(grpc::Status::OK);
        });
}

void TVcsServer::ScheduleGetObject(const TString& hash, TGetObjectCallback cb) {
    FetchPool->SafeAddFunc([this, hash, cb] ()
          {
              TString data;

              if (Lookup(hash, data)) {
                  cb(hash, data, grpc::Status::OK);

              } else {
                  AsyncLookups.ScheduleLookup(hash, 1, cb);
              }
          }
    );
}

TString TVcsServer::HgId(const TString& name) {
    const TString cmd = Sprintf(
        "%s id --id -r %s %s --template {node} --ssh \"ssh -l %s -i %s\"",
        HgBinaryPath.data(),
        name.data(),
        HgServer.data(),
        HgUser.data(),
        HgKey.data()
    );

    TStringStream out;
    {
        TPipeInput pipeInput(cmd);
        TransferData(&pipeInput, &out);
    }

    return HexDecode(out.ReadAll());
}

TFuture<ui64> TVcsServer::AsyncGetSvnHead() {
    auto promise = NewPromise<ui64>();

    {
        auto g(Guard(Lock_));
        if (RevisionWaitList_.empty()) {
            RevisionWaitList_.push_back(promise);
        } else {
            RevisionWaitList_.push_back(promise);
            return promise.GetFuture();
        }
    }

    FetchPool->SafeAddFunc([this] ()
        {
            try {
                AtomicSet(LastKnownSvnHead_, FromString<ui64>(Client->Get(SvnHeadPath).AsString()));
            } catch (...) {
            }

            TVector<TPromise<ui64>> cbs;

            {
                auto g(Guard(Lock_));
                cbs.swap(RevisionWaitList_);
            }

            for (auto ci = cbs.begin(); ci != cbs.end(); ++ci) {
                ci->SetValue(AtomicGet(LastKnownSvnHead_));
            }
        });

    return promise.GetFuture();
}

NThreading::TFuture<TString> TVcsServer::AsyncGetHgId(const TString& name) {
    auto promise = NewPromise<TString>();

    {
        auto g(Guard(HgLock));

        TVector<NThreading::TPromise<TString> > promises;

        auto ins = HgIdWaitLists.insert(std::make_pair(name, std::move(promises)));

        if (ins.second) {
            ins.first->second.push_back(promise);
        } else {
            ins.first->second.push_back(promise);
            return promise.GetFuture();
        }
    }

    FetchPool->SafeAddFunc([this, name] () {
        TString cs;

        try {
            cs = HgId(name);
        } catch (const yexception& e) {
            Trace(TGenericError(e.what()));
        }

        TVector<TPromise<TString> > cbs;

        {
            auto g(Guard(HgLock));
            cbs.swap(HgIdWaitLists[name]);
            HgIdWaitLists.erase(name);
        }

        for (auto ci = cbs.begin(); ci != cbs.end(); ++ci) {
            ci->SetValue(cs);
        }
    });

    return promise.GetFuture();
}

void TVcsServer::WaitObjects2() {
    new TObject2Request(this, &Service_, CQ_.get());
}

void TVcsServer::WaitObjects3() {
    new TObject3Request(this, &Service_, CQ_.get());
}

void TVcsServer::WaitWalk() {
    new TWalkRequest(this, &Service_, CQ_.get());
}

void TVcsServer::WaitSvnHead() {
    new TSvnHeadRequest(this, &Service_, CQ_.get());
}

void TVcsServer::WaitHgId() {
    new THgIdRequest(this, &Service_, CQ_.get());
}

void TVcsServer::WaitPush() {
    new TPushRequest(this, &Service_, CQ_.get());
}

void TVcsServer::WaitNotifyNewObjects() {
    new TNotifyNewObjectsRequest(this, &Service_, CQ_.get());
}

void TVcsServer::DoExecute() {
    WaitObjects2();
    WaitObjects3();
    WaitWalk();
    WaitSvnHead();
    WaitHgId();
    WaitPush();
    WaitNotifyNewObjects();

    while (true) {
        void* tag;
        bool ok;

        if (CQ_->Next(&tag, &ok)) {
            IQueueEvent* const ev(static_cast<IQueueEvent*>(tag));

            if (/*Finishing() || */!ev->Execute(ok)) {
                ev->DestroyRequest();
            }
        } else {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace NAapi;
