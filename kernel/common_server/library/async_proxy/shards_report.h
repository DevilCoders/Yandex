#pragma once

#include "message.h"
#include "report.h"
#include "shard.h"

#include <kernel/common_server/library/searchserver/simple/http_status_config.h>

#include <search/meta/scatter/source.h>

#include <library/cpp/neh/multiclient.h>

#include <util/generic/ptr.h>
#include <library/cpp/deprecated/atomic/atomic.h>

template <class TReport>
class THttpReplyData {
protected:
    using TReportPtr = TAtomicSharedPtr<TReport>;

protected:
    mutable TReportPtr Report;
    ui32 HttpCode = HTTP_INTERNAL_SERVER_ERROR;
    THttpHeaders Headers;

public:
    THttpReplyData() {
        Report = MakeAtomicShared<TReport>();
    }

    ui32 GetHttpCode() const {
        return HttpCode;
    }

    bool HasReport() const {
        return !!Report;
    }

    const TReport& GetReportSafe() const {
        CHECK_WITH_LOG(!!Report);
        return *Report;
    }

    TReport& MutableReport() const {
        return *Report;
    }

    TReportPtr GetReport() const {
        CHECK_WITH_LOG(Report);
        return Report;
    }

    const THttpHeaders& GetHeaders() const {
        return Headers;
    }
};

template <class TReport>
class TShardReport: public THttpReplyData<TReport> {
private:
    using TBase = THttpReplyData<TReport>;
    using TReportPtr = typename TBase::TReportPtr;

private:
    const ui32 ShardId;
    const TString SourceInfo;
    using TBase::HttpCode;
    using TBase::Report;
    using TBase::Headers;

    TAdaptiveLock Lock;
    TAtomic ResultReady;

protected:
    IEventLogger* EventLogger = nullptr;
    IShardDelivery* ShardInfo = nullptr;

protected:
    virtual bool ParseReport(const TString& data, const TString& firstLine, TReport& result, ui32& httpCode) = 0;
    virtual bool ParseErrorReport(const TString& /*data*/, TReport& /*result*/, const ui32 /*httpCode*/) {
        return true;
    }

    bool SetReport(TReportPtr report, ui32 code, const THttpHeaders& headers, bool error = false) {
        TAtomicBase ready = error ? 0 : 1;
        if (!AtomicCas(&ResultReady, ready, 0)) {
            return false;
        }

        auto hdrs = headers;
        auto guard = Guard(Lock);
        if (error && AtomicGet(ResultReady)) {
            return false;
        }

        Headers = std::move(hdrs);
        HttpCode = code;
        Report = std::move(report);
        return true;
    }

public:
    TShardReport(const THttpStatusManagerConfig& httpCodesConfig, IShardDelivery* shardInfo)
        : ShardId(shardInfo->GetShardId())
        , SourceInfo(shardInfo->GetSource()->Descr)
        , ResultReady(0)
        , EventLogger(shardInfo->GetOwner()->GetReportBuilder().GetEventLogger())
        , ShardInfo(shardInfo)
    {
        HttpCode = httpCodesConfig.IncompleteStatus;
    }

    virtual ~TShardReport() {
    }

    ui32 GetShardId() const {
        return ShardId;
    }

    const TString& GetSourceInfo() const {
        return SourceInfo;
    }

    virtual void OnFailRequest(const NNeh::IMultiClient::TEvent::TType /*evType*/, const NNeh::TResponse* /*response*/) {
    }

    virtual void OnSystemError(const NNeh::TResponse* /*response*/) {
    }

    virtual bool AddInfo(const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) final {
        bool result = false;
        if (evType == NNeh::IMultiClient::TEvent::Response) {
            auto report = MakeAtomicShared<TReport>();
            ui32 httpCode;
            if (response->IsError()) {
                if (response->GetErrorType() == NNeh::TError::TType::ProtocolSpecific) {
                    auto code = response->GetErrorCode();
                    bool error = IsServerError(code);
                    result = ParseErrorReport(response->Data, *report, code) && SetReport(report, code, response->Headers, error);
                } else if (response->GetErrorType() == NNeh::TError::TType::UnknownType) {
                    OnSystemError(response);
                }
            } else {
                if (!AtomicGet(ResultReady) && ParseReport(response->Data, response->FirstLine, *report, httpCode)) {
                    result = SetReport(report, httpCode, response->Headers);
                }
            }
        } else {
            OnFailRequest(evType, response);
        }
        return result;
    }
};

template <class TShardResult>
class TShardsReportBuilder: public IReportBuilder {
private:
    TAtomic RepliesCounter = 0;
    TAtomic ReadyShards = 0;

protected:
    TVector<TShardResult> ShardsResults;

protected:
    bool IsIncompleteResponse() const {
        return AtomicGet(ReadyShards) != (i64)ShardsResults.size();
    }

    virtual TShardResult BuildShardResult(IShardDelivery* shardInfo) = 0;

    virtual void DoAddShardInfo(IShardDelivery* shardInfo) final {
        ShardsResults.push_back(BuildShardResult(shardInfo));
    }

    virtual bool DoAddResponse(const IShardDelivery* shardInfo, const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) final {
        if (ShardsResults[GetShardIndex(shardInfo)].AddInfo(evType, response)) {
            if (AtomicIncrement(ReadyShards) == (i64)ShardsResults.size()) {
                CheckReply();
            }
            return true;
        }
        return false;
    }

public:
    const TVector<TShardResult>& GetResults() const {
        return ShardsResults;
    }
};
