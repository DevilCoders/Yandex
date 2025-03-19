#pragma once
#include <kernel/common_server/library/async_proxy/shards_report.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/json/json_value.h>
#include <kernel/common_server/library/neh/server/neh_server.h>
#include <kernel/common_server/library/async_proxy/shard_source.h>
#include <kernel/common_server/library/async_proxy/addr.h>
#include <search/session/logger/logger.h>

namespace NAPHelper {

    extern TAtomic Counter5xx;
    extern TAtomic CounterDisconnect;

    void CheckRequestsFinishes(const TDuration timeout);

    class THttpShardResult: public TShardReport<TString> {
    protected:
        virtual void OnSystemError(const NNeh::TResponse* response) override {
            if (TShardDelivery::IsConnectionProblem(response)) {
                AtomicIncrement(CounterDisconnect);
            }
        }

        virtual bool ParseReport(const TString& data, const TString& /*firstLine*/, TString& result, ui32& httpCode) override {
            TStringInput is(data);
            THttpInput hi(&is);
            httpCode = ParseHttpRetCode(hi.FirstLine());
            if (httpCode / 100 == 5) {
                AtomicIncrement(Counter5xx);
                return false;
            }
            result = hi.ReadAll();
            return true;
        }
    public:
        THttpShardResult(const THttpStatusManagerConfig& httpCodesConfig, IShardDelivery* shardInfo)
            : TShardReport<TString>(httpCodesConfig, shardInfo) {

        }
    };

    class TEventLogDumper: public ILogFrameEventVisitor {
    private:
        TStringStream Log;

        virtual void Visit(const TEvent& event) override {
            Log << event.ToString() << Endl;
        }

    public:
        TString Print() const {
            return Log.Str();
        }
    };

    void DumpLog(const TString& logStr);

    class TReportBuilderPrint: public TShardsReportBuilder<THttpShardResult> {
    private:
        using TBase = TShardsReportBuilder<THttpShardResult>;
        THttpStatusManagerConfig HttpCodesConfig;
        TSet<ui32> ResultCodes;
        TSelfFlushLogFramePtr EventLogFrame;
        mutable TEventLogger EventLogger;
    protected:

        THttpShardResult BuildShardResult(IShardDelivery* shardInfo) override {
            return THttpShardResult(HttpCodesConfig, shardInfo);
        }

        virtual void DoOnReplyReady() const override {
            TStringStream log;
            TEventLogDumper dumper;
            EventLogFrame->VisitEvents(dumper, NEvClass::Factory());
            log << "Dump event log ..." << Endl;
            log << dumper.Print();
            log << "Dump event log ... OK" << Endl;

            NJson::TJsonValue report(NJson::JSON_MAP);
            TSet<ui32> codesReplies;
            for (auto&& i : ShardsResults) {
                report[i.GetShardId()]["report"] = *i.GetReport();
                report[i.GetShardId()]["code"] = i.GetHttpCode();
                codesReplies.insert(i.GetHttpCode());
                CHECK_WITH_LOG(ResultCodes.empty() || ResultCodes.contains(i.GetHttpCode())) << log.Str() << Endl << ResultCodes.size() << "/" << i.GetHttpCode();
            }
            log << report.GetStringRobust() << Endl;
            INFO_LOG << log.Str();
            CHECK_WITH_LOG(ResultCodes.empty() || codesReplies.size() == ResultCodes.size()) << ResultCodes.size() << "/" << codesReplies.size();
        }

        virtual IEventLogger* GetEventLogger() const override {
            return &EventLogger;
        }

    public:
        TReportBuilderPrint(const TSet<ui32>& codes) {
            EventLogFrame = MakeIntrusive<TSelfFlushLogFrame>(true);
            EventLogFrame->ForceDump();
            EventLogger.AssignLog(EventLogFrame);

            ResultCodes = codes;
        }

        TReportBuilderPrint() = default;

    };

    class TSlowNehServer: public NUtil::TAbstractNehServer {
    private:
        TDuration Pause = TDuration::Zero();
        ui32 Code = 200;
        TThreadPool Queue;
        bool CheckCgi = false;
        TString Reply = "REPLY";
        bool NeedHeader = true;
        class TSlowReply: public IObjectInQueue {
        private:
            TDuration Pause = TDuration::Zero();
            NNeh::IRequestRef Request;
            ui32 Code = 200;
            ui32 Step = 0;
            IThreadPool* Queue;
            const TString Reply;
            const bool NeedHeader;
        public:
            TSlowReply(const TDuration pause, NNeh::IRequestRef req, const ui32 code, IThreadPool* queue, const TString& reply, const bool needHeader)
                : Pause(pause)
                , Request(req)
                , Code(code)
                , Queue(queue)
                , Reply(reply)
                , NeedHeader(needHeader)
            {
            }

            virtual void Process(void* /*threadSpecificResource*/) {
                if (++Step == 1) {
                    CHECK_WITH_LOG(Queue->AddAndOwn(THolder(this)));
                } else {
                    Sleep(Pause);
                    NNeh::TDataSaver saver;
                    if (NeedHeader) {
                        saver << "HTTP/1.1 " << Code << " OK";
                        saver << Endl;
                        saver << Endl;
                    }
                    saver << Reply;
                    Request->SendReply(saver);
                }
            }

        };

    protected:
        virtual THolder<IObjectInQueue> DoCreateClientRequest(ui64 /*id*/, NNeh::IRequestRef req) {
            if (CheckCgi) {
                CHECK_WITH_LOG(req->Data().size());
            }
            return MakeHolder<TSlowReply>(Pause, req, Code, &Queue, Reply, NeedHeader);
        }
    public:

        void SetReply(const TString& reply) {
            Reply = reply;
        }

        const TString& GetReply() const {
            return Reply;
        }

        void SetCode(const ui32 code) {
            Code = code;
        }

        void SetNeedHeader(const bool value) {
            NeedHeader = value;
        }

        TSlowNehServer(const TOptions& options, const TDuration pause, const ui32 code, bool checkCgi = false)
            : NUtil::TAbstractNehServer(options)
            , Pause(pause)
            , Code(code)
            , CheckCgi(checkCgi) {
            Queue.Start(options.nThreads);
        }

        ~TSlowNehServer() {
            Queue.Stop();
            Stop();
        }
    };

    using TTrivialShardISource = NAsyncProxyMeta::TSimpleShard;

    TAtomicSharedPtr<TSlowNehServer> BuildServer(const ui32 port, const ui32 threads, const TDuration pause, const ui32 code, TString scheme = "http", bool check = false);

}
