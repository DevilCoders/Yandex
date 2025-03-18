#pragma once
#include <util/thread/pool.h>
#include <library/cpp/threading/serial_postprocess_queue/serial_postprocess_queue.h>
#include <search/begemot/server/worker.h>
#include <mapreduce/yt/interface/io.h>
#include <apphost/lib/service_testing/service_testing.h>

enum class EBegemotMode {
    Normal,
    Cgi,
    Direct
};

struct TBegemotErrorEvent {
    TString type, text;
    TMaybe<TString> rule, what; // for TRuleError
    TMaybe<int> code; // for TRequestError
};

class TProcessRowTask : public TSerialPostProcessQueue::IProcessObject {
private:
    NBg::TWorker& Worker;
    NYT::TNode Request;
    NYT::TTableWriter<NYT::TNode> Writer;
    TString BegemotAnswer;
    TVector<TBegemotErrorEvent> Events;

    EBegemotMode Mode;
    const NJson::TJsonValue& BegemotConfig;
    TString ShardName;
    TString QueryColumn;
    TString RegionColumnOrValue;
    bool IsRegionValue;
    TString AppendCgi;

public:
    TProcessRowTask(
        NBg::TWorker& worker,
        const NYT::TNode& request,
        NYT::TTableWriter<NYT::TNode> writer,
        EBegemotMode mode,
        const NJson::TJsonValue& begemotConfig,
        const TStringBuf& shardName,
        const TStringBuf& queryColumn,
        const TStringBuf& regionColumnOrValue,
        bool isRegionValue,
        const TStringBuf& appendCgi
    );

    void ParallelProcess(void*) override;
    void SerialProcess() override;

private:
    THolder<TCgiParameters> InitCgiMode() const;
    NJson::TJsonValue InitNormalMode() const;
    THolder<TCgiParameters> InitDirectMode() const;
    static void AppendBegemotConfig(NJson::TJsonValue& ctx, const NJson::TJsonValue& begemotConfig);

    void FillNormalParameters(NYT::TNode& result);
    void FillShardParameters(NYT::TNode& result);
};

class TProtobufAwareTestContext : public NAppHost::NService::TTestContext {
private:
    NJson::TJsonValue Result;
    THashSet<TString> ProtobufTypes;
public:
    using TTestContext::TTestContext;
    using TTestContext::Flush;

    void AddProtobufItem(
        const google::protobuf::Message& item,
        TStringBuf type,
        const NAppHost::EContextItemKind kind = NAppHost::EContextItemKind::Output
    );

    const NJson::TJsonValue& GetResult();
};
