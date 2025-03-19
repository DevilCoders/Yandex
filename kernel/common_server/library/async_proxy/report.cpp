#include "report.h"
#include "shard.h"

#include <search/session/logger/logger.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/string_utils/base64/base64.h>

ui32 IReportBuilder::GetShardIndex(const IShardDelivery* shard) const {
    auto it = RegisteredIndex.find(shard);
    CHECK_WITH_LOG(it != RegisteredIndex.end());
    return it->second;
}

void IReportBuilder::OnReplyReady() {
    ReplyReady = true;
    DoOnReplyReady();
    EventReplyReady.Signal();
}

IReportBuilder::~IReportBuilder() {
    CHECK_WITH_LOG(ReplyReady);
}

bool IReportBuilder::CheckReply() {
    if (AtomicIncrement(RepliesCounter) == 1) {
        OnReplyReady();
        return true;
    }
    return false;
}

void IReportBuilder::AddShardInfo(IShardDelivery* shardInfo) {
    CHECK_WITH_LOG(ShardIds.insert(shardInfo->GetShardId()).second);
    CHECK_WITH_LOG(!RegisteredIndex.contains(shardInfo));
    RegisteredIndex[shardInfo] = RegisteredIndex.size();
    DoAddShardInfo(shardInfo);
}

bool IReportBuilder::AddResponse(const IShardDelivery* shard, const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) {
    IEventLogger* logger = GetEventLogger();
    if (logger) {
        if (evType == NNeh::IMultiClient::TEvent::Response) {
            logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), response->Request.Addr + response->Request.Data, "RECV"));
        } else {
            if (response) {
                logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), response->Request.Addr + response->Request.Data, "timeouted"));
            } else {
                logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), "", "timeouted"));
            }
        }
    }
    if (DoAddResponse(shard, evType, response)) {
        if (logger) {
            CHECK_WITH_LOG(response);
            logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), response->Request.Addr + response->Request.Data, "OK"));
        }
        return true;
    } else {
        if (logger) {
            if (!response) {
                CHECK_WITH_LOG(evType == NNeh::IMultiClient::TEvent::Timeout);
                logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), "", "INCORRECT"));
            } else {
                if (!!response->GetSystemErrorCode()) {
                    logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), response->Request.Addr + response->Request.Data, "SYSERROR:" + ToString(response->GetSystemErrorCode())));
                } else {
                    logger->LogEvent(NEvClass::TStageMessage(shard->GetShardId(), response->Request.Addr + response->Request.Data, "INCORRECT:" + Base64Encode(response->Data)));
                }
            }
        }
        return false;
    }
}
