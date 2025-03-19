#pragma once

#include "http_request.h"
#include "neh_request.h"

#include <kernel/common_server/library/async_proxy/async_delivery.h>
#include <kernel/common_server/library/async_proxy/shards_report.h>
#include <kernel/common_server/library/async_proxy/shard_source.h>
#include <kernel/common_server/library/metasearch/simple/config.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/map.h>

namespace NNeh {
    using THttpReply = NUtil::THttpReply;

    class THttpAsyncReport: public TShardReport<TString> {
    private:
        using TBase = TShardReport<TString>;

    protected:
        virtual bool ParseErrorReport(const TString& data, TString& result, const ui32 /*httpCode*/) override;
        virtual bool ParseReport(const TString& data, const TString& firstLine, TString& result, ui32& httpCode) override;

        virtual void OnSystemError(const NNeh::TResponse* response) override;

    public:
        class ICallback {
        protected:
            const TInstant StartTime = Now();

        public:
            using TPtr = TAtomicSharedPtr<ICallback>;

        public:
            virtual ~ICallback() = default;

            virtual void OnResponse(const TVector<THttpAsyncReport>& reports) = 0;
        };

        class THandlerCallback
            : public ICallback
            , public IObjectInQueue
        {
        private:
            IThreadPool& Handler;

        protected:
            using ICallback::StartTime;
            TVector<THttpReplyData<TString>> Reports;

        public:
            THandlerCallback(IThreadPool& handler)
                : Handler(handler)
            {
            }

            virtual void OnResponse(const TVector<THttpAsyncReport>& reports) override;
        };

    public:
        using TBase::TBase;
    };

    class THttpClient {
    public:
        class TSourceData: public THttpRequestBuilder {
        private:
            NSimpleMeta::TConfig Config;
            THolder<NScatter::ISource> Source;
            const TString Name;

        private:
            TString GetScriptWithReasks() const;

        public:
            using TPtr = TAtomicSharedPtr<TSourceData>;

        public:
            TSourceData(const TString& host, const ui16 port, const NSimpleMeta::TConfig& config, const TString& name, const bool isHttps = false, const TString& cert = "", const TString& certKey = "");
            ~TSourceData();

            const TString& GetName() const {
                return Name;
            }

            NScatter::ISource* GetSource() const {
                return Source.Get();
            }

            const NSimpleMeta::TConfig& GetConfig() const {
                return Config;
            }
        };

    private:
        class THttpRequestShard;
        class TShardsSimpleReportBuilder;

    private:
        TRWMutex SourcesLock;
        TMap<TString, TSourceData::TPtr> Sources;
        TAsyncDelivery::TPtr AsyncDelivery;
        TAsyncDelivery::TPtr Delivery;
        IEventLog* EventLog = nullptr;

    public:
        THttpClient(TAsyncDelivery::TPtr delivery, IEventLog* eventLog = nullptr);
        THttpClient(const NSimpleMeta::TConfig& config = NSimpleMeta::TConfig::ForRequester(), TAsyncDelivery::TPtr ad = nullptr);
        THttpClient(const TStringBuf url, const NSimpleMeta::TConfig& config = NSimpleMeta::TConfig::ForRequester(), TAsyncDelivery::TPtr ad = nullptr);
        ~THttpClient();

    public:
        class TGuard {
        private:
            TAtomicSharedPtr<TShardsSimpleReportBuilder> ReportBuilder;

        public:
            TGuard(TAtomicSharedPtr<TShardsSimpleReportBuilder> reportBuilder);
            TGuard();
            ~TGuard();

            bool IsEmpty() const {
                return !ReportBuilder;
            }

            IReportBuilder::TPtr GetReportBuilder();
            const TVector<THttpAsyncReport>& GetReports();
            TGuard& Wait(const TInstant deadline = TInstant::Max());
        };

    public:
        THttpClient& RegisterSource(const TString& name, const TString& host, const ui16 port, const NSimpleMeta::TConfig& config, const bool isHttps = false, const TString& cert = "", const TString& certKey = "");
        THttpClient& RegisterSource(const TStringBuf url, const NSimpleMeta::TConfig& config = NSimpleMeta::TConfig::ForRequester(), const TString& name = {});
        THttpClient& RegisterSource(TSourceData::TPtr data);
        bool HasSource(const TString& name) const;
        TSourceData::TPtr GetSource(const TString& name) const;
        TSourceData::TPtr GetUniqueSource() const;

        NUtil::THttpReply SendMessageSync(const THttpRequest& request, const TInstant deadline = TInstant::Zero()) const;
        NThreading::TFuture<NUtil::THttpReply> SendAsync(const THttpRequest& request, const TInstant deadline = TInstant::Zero()) const;
        NThreading::TFuture<NUtil::THttpReply> SendAsync(const TString& name, const THttpRequest& request, const TInstant deadline = TInstant::Zero()) const;

        TMap<TString, NUtil::THttpReply> SendPack(const TMap<TString, THttpRequest>& requests,
                                                  const TInstant deadline,
                                                  const ui32 maxInFlight) const;

        TMap<TString, NUtil::THttpReply> SendPack(const TMap<TString, THttpRequest>& requests,
                                                  const TInstant deadline,
                                                  const ui32 maxInFlight,
                                                  const TDuration reqTimeout) const;

        TVector<NUtil::THttpReply> SendPack(const TVector<THttpRequest>& requests,
                                            const TInstant deadline,
                                            const ui32 maxInFlight,
                                            const TDuration reqTimeout) const;

        TGuard SendPtr(const TMultiMap<TString, THttpRequest>& requests, const TInstant deadline, TAtomicSharedPtr<THttpAsyncReport::ICallback> callback) const;
        TGuard SendPtr(const THttpRequest& request, const TInstant deadline, TAtomicSharedPtr<THttpAsyncReport::ICallback> callback) const;

        TGuard Send(const TMultiMap<TString, THttpRequest>& requests, const TInstant deadline, THolder<THttpAsyncReport::ICallback>&& callback) const;
        TGuard Send(const THttpRequest& request, const TInstant deadline, THolder<THttpAsyncReport::ICallback>&& callback) const;

        TGuard Send(const TMultiMap<TString, THttpRequest>& requests, const TInstant deadline, THttpAsyncReport::ICallback* callback = nullptr) const;
        TGuard Send(const THttpRequest& request, const TInstant deadline, THttpAsyncReport::ICallback* callback = nullptr) const;

    private:
        TInstant GetDeadline(const TString& source, const NSimpleMeta::TConfig* metaConfig) const;
    };
}
