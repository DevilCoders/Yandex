#include "pq_reader.h"
#include "log_converter.h"
#include "ydb_writer.h"
#include "monitoring.h"

#include <library/cpp/lfalloc/alloc_profiler/profiler.h>
#include <library/cpp/getopt/opt.h>
#include <library/cpp/logger/priority.h>

#include <util/generic/stack.h>
#include <util/generic/queue.h>
#include <util/generic/ptr.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/system/mutex.h>
#include <util/system/condvar.h>
#include <util/system/thread.h>
#include <util/stream/file.h>
#include <util/thread/pool.h>


using namespace NLastGetopt;
using namespace NPersQueue;


template <class TItem>
class TCtxPool : TNonCopyable {
public:
    explicit TCtxPool(ui64 maxInFlight)
        : MaxInFlight(maxInFlight)
    {}

    TItem* Get() {
        TItem* item = nullptr;

        {
            TGuard<TMutex> g(FreeListLock);
            if (InFlight >= MaxInFlight) {
                FreeListNotEmpty.WaitI(FreeListLock);
            }
            AtomicIncrement(InFlight);

            if (!FreeList.empty()) {
                item = FreeList.top().Release();
                FreeList.pop();
            }
        }

        if (!item) {
            item = new TItem;
        }

        return item;
    }

    void Return(TItem* item) {
        item->Clear();
        AtomicDecrement(InFlight);
        TGuard<TMutex> g(FreeListLock);
        FreeList.push(THolder<TItem>(item));
        if (InFlight < MaxInFlight) {
            FreeListNotEmpty.Signal();
        }
    }

private:
    TMutex FreeListLock;
    TCondVar FreeListNotEmpty;
    TStack<THolder<TItem>> FreeList;
    TAtomic InFlight = 0;
    TAtomic MaxInFlight;
};

struct TWriterSettings {
    TString TablePath;
    TVector<TString> KeyColumns;
    TDuration NewTableInterval;
    TString TimestampColumn;
    TString TableProfile;
};


class TLogPipeline {
public:
    TLogPipeline(ui64 ydbMaxInFlight, ui64 miniBatchSize)
        : CtxPool(100)
        , TaskQueue()
        , RetryScheduler(TaskQueue)
        , YdbInFlight(ydbMaxInFlight)
        , MiniBatchSize(miniBatchSize)
        , ProfileIdx(0)
    {}

    void Start(const NPersQueue::TConsumerSettings& settings, TString logFormat, NYdb::TDriver& driver,
               const TWriterSettings& writerSettings, size_t threadCount, TIntrusivePtr<NPersQueue::TCerrLogger> logger)
    {
        signal(SIGTERM, &SigHandler);
        signal(SIGINT, &SigHandler);

        if (!writerSettings.TimestampColumn.empty() && !FindPtr(writerSettings.KeyColumns, writerSettings.TimestampColumn)) {
            ythrow yexception() << "ts column [" << writerSettings.TimestampColumn << "] must be included in primary key";
        }

        PqReader.Reset(new TPqReader(settings, logger));
        LogConverter.Reset(new TLogConverter(logFormat, writerSettings.NewTableInterval, writerSettings.TimestampColumn));

        TYdbWriter::TSchema schema;
        auto columnMap = LogConverter->GetColumnList();
        for (auto cit = columnMap.begin(); cit != columnMap.end(); ++cit) {
            if (cit->first != "_logfeller_timestamp" && cit->first != "iso_eventtime") {
                schema.Columns[cit->first] = cit->second;
            }
        }

        schema.KeyColumns = writerSettings.KeyColumns;

        YdbWriter.Reset(new TYdbWriter(driver, writerSettings.TablePath, writerSettings.TableProfile,
                                       YdbInFlight.GetLimit() + 10, std::move(schema)));

        TaskQueue.Start(threadCount);
    }

    void Run(bool profileMem) {
        if (profileMem) {
            StartProfiling();
        }

        while (!Interrupted) {
            THolder<TPqBatchCtx> ctx = THolder(CtxPool.Get());
            ctx->Self = this;

            PqReader->ReadBatch().Swap(&ctx->PqMessage);
            ctx->StartTime = TInstant::Now();

            bool added = TaskQueue.Add(ctx.Release());
            Y_VERIFY(added, "Failed to add a task to queue");

            if (DumpProfile) {
                SaveProfile();
            }
        }
    }

    void Stop() {
        RetryScheduler.Stop();
        TaskQueue.Stop();
    }

private:
    struct TPqBatchCtx : IObjectInQueue {
        TLogPipeline* Self = nullptr;
        NPersQueue::TReadResponse PqMessage;
        TInstant StartTime;

        TDeque<std::pair<TString, TVector<TLogConverter::TLogRow>>> Batches;

        TVector<ui64> CookiesToCommit;
        TAtomic YdbRequestsInFlight = 0;

        void Clear() {
            Self = nullptr;
            PqMessage.Clear();
            Batches.clear();
            CookiesToCommit.clear();
            YdbRequestsInFlight = 0;
        }

        void Process(void* threadSpecificResource) override {
            Y_UNUSED(threadSpecificResource);
            return Self->ProcessPqMessage(this);
        }
    };

    struct TYdbRetryCtx : IObjectInQueue {
        TPqBatchCtx* Ctx = nullptr;
        TVector<TLogConverter::TLogRow> Streams;
        size_t BatchIdx = -1;
        size_t RetryCount = 0;
        TInstant RetryAt;

        void Process(void* threadSpecificResource) override {
            Y_UNUSED(threadSpecificResource);
            if (BatchIdx >= 0) {
                return Ctx->Self->SaveBatch(Ctx, BatchIdx, THolder(this), Streams);
            } else {
                return Ctx->Self->SaveStreams(Ctx, THolder(this), Streams);
            }
        }
    };

    class TRetryScheduler {
    public:
        TRetryScheduler(IThreadPool& processingQueue)
            : ProcessingQueue(processingQueue)
            , SchedulerThread(SchedulerProc, this)
            , Interrupted(false)
        {
            SchedulerThread.Start();
        }

        ~TRetryScheduler() {
            Y_VERIFY(ItemsByDeadline.empty());
        }

        void Add(THolder<TYdbRetryCtx>&& item) {
            Monitoring->BatchRetriesC->Inc();

            TGuard<TMutex> g(QueueMutex);

            TInstant prevWakeUp = ItemsByDeadline.empty() ? TInstant::MicroSeconds(-1) : ItemsByDeadline.top()->RetryAt;
            TInstant newWakeUp = item->RetryAt;

            ItemsByDeadline.push(item.Release());

            if (newWakeUp < prevWakeUp) {
                // Need to reschedule wake up if the new item got to the head of the queue
                WakeUpEvent.Signal();
            }
        }

        void Stop() {
            Interrupt();
            SchedulerThread.Join();

            while (!ItemsByDeadline.empty()) {
                auto item = ItemsByDeadline.top();
                ItemsByDeadline.pop();
                bool ok = ProcessingQueue.Add(item);
                Y_VERIFY(ok);
            }
        }

    private:
        void Interrupt() {
            TGuard<TMutex> g(QueueMutex);
            Interrupted = true;
            WakeUpEvent.Signal();
        }

        static void* SchedulerProc(void* p) {
            TRetryScheduler* This = (TRetryScheduler*)p;
            This->SchedulerProcImpl();
            return nullptr;
        }

        void SchedulerProcImpl() {
            TVector<TYdbRetryCtx*> readyItems;
            while(!Interrupted) {
                {
                    TGuard<TMutex> g(QueueMutex);
                    TInstant nextWakeUp = ItemsByDeadline.empty() ?
                                TInstant::Now() + TDuration::Minutes(1) : ItemsByDeadline.top()->RetryAt;

                    WakeUpEvent.WaitD(QueueMutex, nextWakeUp);

                    if (Interrupted)
                        break;

                    TInstant now = TInstant::Now();

                    if (now < nextWakeUp)
                        continue;

                    while (!ItemsByDeadline.empty() && ItemsByDeadline.top()->RetryAt <= now) {
                        auto item = ItemsByDeadline.top();
                        ItemsByDeadline.pop();
                        readyItems.push_back(item);
                    }
                }
                for (auto item : readyItems) {
                    bool ok = ProcessingQueue.Add(item);
                    Y_VERIFY(ok);
                }
                readyItems.clear();
            }
        }

    private:
        struct TDeadlineGreater {
            bool operator ()(const TYdbRetryCtx* a, const TYdbRetryCtx* b) {
                return a->RetryAt > b->RetryAt;
            }
        };

    private:
        TMutex QueueMutex;
        TCondVar WakeUpEvent;
        TPriorityQueue<TYdbRetryCtx*, TVector<TYdbRetryCtx*>, TDeadlineGreater> ItemsByDeadline;
        IThreadPool& ProcessingQueue;
        TThread SchedulerThread;
        bool Interrupted;
    };

    class TInFlightLimiter : TNonCopyable {
    public:
        explicit TInFlightLimiter(ui64 limit)
            : Limit(limit)
            , InFlight(0)
            , Waiting(0)
        {
            Monitoring->YdbInFlightLimitG->Set(Limit);
        }

        ui64 GetLimit() const {
            return Limit;
        }

        void Acquire() {
            TGuard<TMutex> g(Lock);
            while (InFlight >= Limit) {
                ++Waiting;
                WakeUp.Wait(Lock);
                --Waiting;
            }
            ++InFlight;

            Monitoring->YdbInFlightCurrentG->Set(InFlight);
        }

        void Release() {
            TGuard<TMutex> g(Lock);
            --InFlight;
            if (Waiting > 0)
                WakeUp.BroadCast();

            Monitoring->YdbInFlightCurrentG->Set(InFlight);
        }

    private:
        const ui64 Limit;
        ui64 InFlight;
        ui64 Waiting;
        TMutex Lock;
        TCondVar WakeUp;
    };

private:
    void SplitIntoMinibatches(TString table, TVector<TLogConverter::TLogRow>& allRows,
                              TDeque<std::pair<TString, TVector<TLogConverter::TLogRow>>>& miniBatches)
    {
        for (auto& r : allRows) {
            if (miniBatches.empty() || miniBatches.back().first != table || miniBatches.back().second.size() >= MiniBatchSize) {
                miniBatches.emplace_back();
                miniBatches.back().first = table;
                miniBatches.back().second.reserve(MiniBatchSize);
            }
            miniBatches.back().second.emplace_back(std::move(r));
        }
    }

    void ProcessPqMessage(TPqBatchCtx* ctx) noexcept {
        if (Interrupted) {
            CtxPool.Return(ctx);
            return;
        }

        Monitoring->BatchAcceptedC->Inc();

        auto& msg = ctx->PqMessage;
        ctx->CookiesToCommit.push_back(msg.GetData().GetCookie());

        THashMap<TString, TVector<TLogConverter::TLogRow>> perTable;
        TVector<TLogConverter::TLogRow> streams;
        LogConverter->Parse(msg, perTable, streams);
        msg.Clear();

        if (perTable.empty()) {
            PqReader->Commit(std::move(ctx->CookiesToCommit));
            CtxPool.Return(ctx);
            return;
        }

        Y_VERIFY(ctx->Batches.empty());

        for (auto& t : perTable) {
            SplitIntoMinibatches(t.first, t.second, ctx->Batches);
        }

        Y_VERIFY(!ctx->Batches.empty());

        auto batchSize = ctx->Batches.size();
        Monitoring->BatchMiniParsedC->Add(batchSize);

        AtomicSet(ctx->YdbRequestsInFlight, batchSize);
        for (size_t i = 0; i < batchSize; ++i) {
            SaveBatch(ctx, i, nullptr, streams);
        }

        Monitoring->BatchMiniProcessedC->Add(batchSize);
    }


    void SaveBatch(TPqBatchCtx* ctx, size_t batchIdx, THolder<TYdbRetryCtx> retryCtx, TVector<TLogConverter::TLogRow> streams) {
        const auto& batch = ctx->Batches[batchIdx];

        YdbInFlight.Acquire();

        YdbWriter->Write(batch.first, batch.second).Apply([this, ctx, batchIdx, rc = retryCtx.Release(), streams] (auto f) noexcept {
            YdbInFlight.Release();

            THolder<TYdbRetryCtx> retryCtx(rc);
            auto result = f.ExtractValue();

            if (!result.IsSuccess()) {
                if (Interrupted)
                    return;

                Cerr << "Error while writing to YDB: " << result.GetStatus() << Endl;

                // Retry the request to YDB after some delay
                if (!retryCtx)
                   retryCtx.Reset(new TYdbRetryCtx);
                retryCtx->Ctx = ctx;
                retryCtx->BatchIdx = batchIdx;
                retryCtx->Streams = streams;
                retryCtx->RetryCount++;
                ui64 delay = 1 << retryCtx->RetryCount;
                delay = 10 + rand() % delay;
                delay = std::min<ui64>(delay, 10000);
                retryCtx->RetryAt = TInstant::Now() + TDuration::MilliSeconds(delay);

                Cerr << "RETRY after " << delay << Endl;

                RetryScheduler.Add(std::move(retryCtx));

                return;
            }

            Monitoring->BatchMiniSavedC->Inc();

            // This batch is finished
            ctx->Batches[batchIdx].second.clear();

            i64 left = AtomicDecrement(ctx->YdbRequestsInFlight);
            if (left == 0) {
                // All minibatches were written
                TDuration processingTime = TInstant::Now() - ctx->StartTime;
                Cerr << "BATCH processing completed, minibatches: " << ctx->Batches.size() << " duration: " << processingTime << Endl;
                Monitoring->BatchProcessedC->Inc();
                PqReader->Commit(std::move(ctx->CookiesToCommit));
                SaveStreams(ctx, nullptr, streams);
                Monitoring->BatchCommittedC->Inc();
            }
        });
    }

    void SaveStreams(TPqBatchCtx* ctx, THolder<TYdbRetryCtx> retryCtx, TVector<TLogConverter::TLogRow> streams) {

        YdbInFlight.Acquire();

        YdbWriter->WriteStreams(streams).Apply([this, ctx, rc = retryCtx.Release(), streams] (auto f) noexcept {
            YdbInFlight.Release();

            THolder<TYdbRetryCtx> retryCtx(rc);
            auto result = f.ExtractValue();

            if (!result.IsSuccess()) {
                if (Interrupted)
                    return;

                Cerr << "Error while writing to YDB: " << result.GetStatus() << Endl;

                // Retry the request to YDB after some delay
                if (!retryCtx)
                    retryCtx.Reset(new TYdbRetryCtx);
                retryCtx->Ctx = ctx;
                retryCtx->BatchIdx = -1;
                retryCtx->Streams = streams;
                retryCtx->RetryCount++;
                ui64 delay = 1 << retryCtx->RetryCount;
                delay = 10 + rand() % delay;
                delay = std::min<ui64>(delay, 10000);
                retryCtx->RetryAt = TInstant::Now() + TDuration::MilliSeconds(delay);

                Cerr << "RETRY after " << delay << Endl;

                RetryScheduler.Add(std::move(retryCtx));

                return;
            }
            CtxPool.Return(ctx);
        });
    }


    static void SigHandler(int) {
        Interrupted = true;
        TPqReader::Interrupt();
    }

    static void SigUsrHandler(int) {
        DumpProfile = true;
    }

    void StartProfiling() {
        signal(SIGUSR1, &SigUsrHandler);
        NAllocProfiler::StartAllocationSampling(true);
    }

    void SaveProfile() {
        DumpProfile = false;
        TString name = Sprintf("/tmp/ydb-sender.profile.%d.%d", getpid(), ProfileIdx++);
        TFileOutput out(name);
        NAllocProfiler::StopAllocationSampling(out, 1000);
        Cerr << "Memory profile saved to " << name << Endl;
        NAllocProfiler::StartAllocationSampling(true);
    }

private:
    THolder<TPqReader> PqReader;
    THolder<TLogConverter> LogConverter;
    THolder<TYdbWriter> YdbWriter;

    TCtxPool<TPqBatchCtx> CtxPool;
    TThreadPool TaskQueue;
    TRetryScheduler RetryScheduler;
    TInFlightLimiter YdbInFlight;
    const size_t MiniBatchSize;
    int ProfileIdx;

    static bool Interrupted;
    static bool DumpProfile;
};

bool TLogPipeline::Interrupted = false;
bool TLogPipeline::DumpProfile = false;


TString CutTrailingEndls(TString token) {
    while(!token.empty() && token.back() == '\n') {
        token.resize(token.size() - 1);
    }
    return token;
}

int main(int argc, const char* argv[]) {
    bool verbose = false;
    bool profileMemory = false;

    TString auth, tvmSecret;
    ui32 tvmClientId = 2006873;
    ui32 tvmDestId = 2001059;

    TConsumerSettings pqSettings;
    pqSettings.UseLockSession = true;
    pqSettings.ReadMirroredPartitions = true;
    pqSettings.MaxCount = 1000;
    pqSettings.MaxSize = 1u << 20;
    pqSettings.Unpack = true;
    pqSettings.MaxUncommittedCount = 10000;
    pqSettings.MaxUncommittedSize = 5u << 30;
    pqSettings.PartsAtOnce = 0;
    pqSettings.MaxMemoryUsage = 1u << 30;
    pqSettings.MaxInflyRequests = 50;

    TString pqLogFormat;

    TString ydbEndpoint;
    TString ydbDatabase;
    TString ydbTokenFile;
    ui64 ydbMaxInFlight;
    ui64 monitoringPort;
    ui64 ydbMiniBatchSize;
    TWriterSettings writerSettings;

    ui32 threadCount = 3;

    THashMap<TString, TString> metricsLabels;

    {
        TOpts opts;
        opts.AddHelpOption('h');
        opts.AddVersionOption();
        opts.AddLongOption('a', "pq-address", "PQ server addr")
            .StoreResult(&pqSettings.Server.Address)
            .Required();
        opts.AddLongOption('p', "pq-port", "PQ server port")
            .StoreResult(&pqSettings.Server.Port)
            .DefaultValue(2135);
        opts.AddLongOption('c', "pq-client", "PQ clientId")
            .StoreResult(&pqSettings.ClientId)
            .Required();
        opts.AddLongOption('t', "pq-topic", "PQ topics to read from, comma-separated")
            .SplitHandler(&pqSettings.Topics, ',')
            .Required();
        opts.AddLongOption('g', "pq-group", "PQ groups to read from, comma-separated")
            .Handler1T<TString>([&] (const TString& val) {
                TVector<TString> groups = SplitString(val, ",");
                for (const auto& g: groups) {
                    pqSettings.PartitionGroups.push_back(FromString<ui32>(g));
                }
            });
        opts.AddLongOption('f', "pq-log-format", "log format name")
            .StoreResult(&pqLogFormat)
            .Required();
        opts.AddLongOption('i', "pq-inflight", "inflight size in bytes")
            .StoreResult(&pqSettings.MaxMemoryUsage)
            .DefaultValue(pqSettings.MaxMemoryUsage)
            .Optional();
        opts.AddLongOption('n', "pq-inflight2", "inflight in requests")
            .StoreResult(&pqSettings.MaxInflyRequests)
            .DefaultValue(pqSettings.MaxInflyRequests)
            .Optional();
        opts.AddLongOption("pq-batch", "max number of message in one batch received from PQ")
            .StoreResult(&pqSettings.MaxCount)
            .DefaultValue(pqSettings.MaxCount)
            .Optional();
        opts.AddLongOption("pq-batch-size", "max size of one bactch received from PQ")
            .StoreResult(&pqSettings.MaxSize)
            .DefaultValue(pqSettings.MaxSize)
            .Optional();
        opts.AddLongOption("pq-uncommitted-cnt", "max number of messages to process be requiring to commit the offset")
            .StoreResult(&pqSettings.MaxUncommittedCount)
            .DefaultValue(pqSettings.MaxUncommittedCount)
            .Optional();
        opts.AddLongOption("pq-uncommitted-size", "max size of messages to process be requiring to commit the offset")
            .StoreResult(&pqSettings.MaxUncommittedSize)
            .DefaultValue(pqSettings.MaxUncommittedSize)
            .Optional();
        opts.AddLongOption('o', "pq-token", "authentication token file for PQ")
            .StoreResult(&auth);
        opts.AddLongOption(0, "pq-tvm-dest-id", "tvm destination id")
            .StoreResult(&tvmDestId)
            .DefaultValue(tvmDestId);
        opts.AddLongOption(0, "pq-tvm-client-id", "tvm client id")
            .StoreResult(&tvmClientId);
        opts.AddLongOption(0, "pq-tvm-secret", "file with tvm secret")
            .StoreResult(&tvmSecret);

        opts.AddLongOption('s', "ydb-endpoint", "address of YDB to store the logs")
            .StoreResult(&ydbEndpoint)
            .Required();
        opts.AddLongOption('d', "ydb-database", "YDB database name to store log tables")
            .StoreResult(&ydbDatabase)
            .Optional();
        opts.AddLongOption("ydb-table-profile", "name of table presets for log tables")
            .StoreResult(&writerSettings.TableProfile)
            .Optional();
        opts.AddLongOption('l', "ydb-path", "path in YDB to store log tables")
            .StoreResult(&writerSettings.TablePath)
            .Required();
        opts.AddLongOption('y', "ydb-token", "authentication token file for YDB")
            .StoreResult(&ydbTokenFile)
            .Optional();
        opts.AddLongOption("ydb-inflight", "max in-flight request to YDB")
            .StoreResult(&ydbMaxInFlight)
            .DefaultValue(100)
            .Optional();
        opts.AddLongOption("ydb-batch-size", "number of rows written to YDB in one request")
            .StoreResult(&ydbMiniBatchSize)
            .DefaultValue(100)
            .Optional();

        opts.AddLongOption('k', "key-columns", "list of fields to be used as key in YDB table, comma-separated")
            .DefaultValue("logGroupId,streamName,ksuid")
            .SplitHandler(&writerSettings.KeyColumns, ',');
        opts.AddLongOption("ts-column", "name of timestamp column that is used for reverse ordering")
            .StoreResult(&writerSettings.TimestampColumn)
            .Optional();
        opts.AddLongOption(0, "new-table-interval", "interval for starting a new table")
            .DefaultValue("1d")
            .StoreResult(&writerSettings.NewTableInterval)
            .Optional();

        opts.AddLongOption('v', "verbose", "verbose")
            .StoreResult(&verbose)
            .DefaultValue("no");
        opts.AddLongOption("profile-mem", "Profile memory usage")
            .StoreResult(&profileMemory)
            .DefaultValue("no");
        opts.AddLongOption('j', "threads", "number of threads used for parsing")
            .StoreResult(&threadCount)
            .Optional();

        opts.AddLongOption(0, "metrics-label", "attach labels to reported metrics")
            .KVHandler([&](TString key, TString value) {
                metricsLabels[key] = value;
            })
            .Optional();

        opts.AddLongOption(0, "monitoring-port", "monitoring port")
                .DefaultValue(8081)
                .StoreResult(&monitoringPort)
                .Optional();

        TOptsParseResult res(&opts, argc, argv);
    }

    Monitoring = MakeAtomicShared<TMonitoringCtx>(static_cast<ui16>(monitoringPort), metricsLabels);
    TIntrusivePtr<TCerrLogger> logger(new TCerrLogger(verbose ? TLOG_DEBUG : TLOG_ERR));

    if (!auth.empty()) {
        pqSettings.CredentialsProvider = CreateOAuthCredentialsProvider(CutTrailingEndls(TFileInput(auth).ReadAll()));
    } else if (!tvmSecret.empty() && tvmClientId != 0) {
        TString alias = "MyAlias";

        NTvmAuth::NTvmApi::TClientSettings tvmSettings;
        tvmSettings.SetSelfTvmId(tvmClientId);
        tvmSettings.EnableServiceTicketsFetchOptions(TFileInput(tvmSecret).ReadAll(), {{alias, tvmDestId}});
        auto tvmLogger = MakeIntrusive<NTvmAuth::TCerrLogger>(verbose ? TLOG_DEBUG : TLOG_ERR);

        //you can use this tmvClient for getting tickets for other services, not just LB
        std::shared_ptr<NTvmAuth::TTvmClient> tvmClient;

        tvmClient = std::make_shared<NTvmAuth::TTvmClient>(tvmSettings, tvmLogger);
        pqSettings.CredentialsProvider = CreateTVMCredentialsProvider(tvmClient, logger, alias);
    }

    auto ydbConfig = NYdb::TDriverConfig()
        .SetClientThreadsNum(6)
        .SetMaxClientQueueSize(0) // unlimited in-flight in SDK
        .SetEndpoint(ydbEndpoint);
    if (!ydbDatabase.empty()) {
        ydbConfig.SetDatabase(ydbDatabase);
    }
    if (!ydbTokenFile.empty()) {
        ydbConfig.SetAuthToken(CutTrailingEndls(TFileInput(ydbTokenFile).ReadAll()));
    }

    {
        NYdb::TDriver ydbDriver(ydbConfig);

        TLogPipeline pipeline(ydbMaxInFlight, ydbMiniBatchSize);

        pipeline.Start(pqSettings, pqLogFormat, ydbDriver, writerSettings, threadCount, logger);
        pipeline.Run(profileMemory);

        ydbDriver.Stop(true);
        pipeline.Stop();
    }

    return 0;
}
