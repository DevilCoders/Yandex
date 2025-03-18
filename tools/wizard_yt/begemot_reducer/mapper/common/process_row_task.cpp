#include "process_row_task.h"
#include <util/string/join.h>
#include <library/cpp/string_utils/quote/quote.h>

using namespace NYT;


class TLogFrameVisitor : public ILogFrameEventVisitor {
    TSet<TEventClass> FilteredEvents;
    TVector<TBegemotErrorEvent>& FoundEvents;

public:
    TLogFrameVisitor(TVector<TBegemotErrorEvent>& foundEvents);
    void Visit(const TEvent &event) override;
};

static const TString* GetLastParam(const TCgiParameters& cgi, TStringBuf name);


TProcessRowTask::TProcessRowTask(
    NBg::TWorker& worker,
    const TNode& request,
    TTableWriter<TNode> writer,
    EBegemotMode mode,
    const NJson::TJsonValue& begemotConfig,
    const TStringBuf& shardName,
    const TStringBuf& queryColumn,
    const TStringBuf& regionColumnOrValue,
    bool isRegionValue,
    const TStringBuf& appendCgi
)
    : Worker(worker)
    , Request(request)
    , Writer(writer)
    , Mode(mode)
    , BegemotConfig(begemotConfig)
    , ShardName(shardName)
    , QueryColumn(queryColumn)
    , RegionColumnOrValue(regionColumnOrValue)
    , IsRegionValue(isRegionValue)
    , AppendCgi(appendCgi)
{
}

void TProcessRowTask::ParallelProcess(void*) {
    try {
        NJson::TJsonValue init = NJson::JSON_ARRAY;
        THolder<TCgiParameters> cgi;
        TString requestText;

        switch(Mode) {
            case EBegemotMode::Cgi:
                cgi = InitCgiMode();
                break;

            case EBegemotMode::Direct:
                cgi = InitDirectMode();
                break;

            default:
                init = InitNormalMode();
                break;
        }

        if (BegemotConfig.GetType() != NJson::JSON_UNDEFINED) {
            AppendBegemotConfig(init, BegemotConfig);
        }

        TProtobufAwareTestContext ctx(init);
        TLogFrameVisitor visitor(Events);

        try {
            Worker(ctx, cgi.Get(), &visitor);
        }
        catch (const yexception& e) {
            ctx.AddFlag("error");
            auto& out = ctx.AddIncompleteItem("begemot").BeginObject();
            out.WriteKey("error_message").WriteString(e.what());
            out.WriteKey("error_code").WriteString("2");
            out.EndObject();
        }

        if (Mode == EBegemotMode::Normal) {
            ctx.Flush();
            BegemotAnswer = ToString(ctx.GetResult());
        } else {
            auto* json = GetLastParam(*cgi, "wizjson");
            BegemotAnswer = json ? TString::Join("[", *json, "]") : TString("[]");
        }
     } catch (const TWithBackTrace<yexception>& e) {
        e.BackTrace()->PrintTo(Cerr);
        throw e;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        Cerr << "Request:\n" << Request["prepared_request"].AsString() << Endl;
        throw;
    }
}

void TProcessRowTask::SerialProcess() {
    try {
        TNode result;
        result["begemot_answer"] = BegemotAnswer;

        if (Mode == EBegemotMode::Direct) {
            result["reqid"] = Request[QueryColumn];
        } else {
            FillNormalParameters(result);
        }

        Writer.AddRow(result, 0);
        int tableIndex = 2;
        for (const auto& event : Events) {
            if (event.type == "TRequestError" && event.code != 21) {
                tableIndex = 1;
            }
        }
        for (const auto& event : Events) {
            auto row = TNode::CreateMap();
            row["reqid"] = result["reqid"];
            row["event_type"] = event.type;
            row["text"] = event.text;
            if (event.rule) {
                row["rule"] = event.rule.GetRef();
            }
            if (event.what) {
                row["what"] = event.what.GetRef();
            }
            if (event.code) {
                row["code"] = event.code.GetRef();
            }
            Writer.AddRow(row, tableIndex);
        }
    } catch(...) {
        Cerr << Join(" ", "Exception in SerialProcess:", CurrentExceptionMessage()) << Endl;
        // TSerialPostProcessQueue executes SerialProcess in TThreadPool with CatchingMode and exceptions are ignored
        abort();
    }
}

THolder<TCgiParameters> TProcessRowTask::InitCgiMode() const {
    THolder<TCgiParameters> cgi;
    const TString& req = Request["prepared_request"].AsString();
    cgi.Reset(new TCgiParameters(req));
    cgi->ReplaceUnescaped("format", "json");
    return cgi;
}

NJson::TJsonValue TProcessRowTask::InitNormalMode() const {
    NJson::TJsonValue init;
    const TString& req = Request["prepared_request"].AsString();
    NJson::ReadJsonFastTree(TStringBuf(req), &init);
    return init;
}

THolder<TCgiParameters> TProcessRowTask::InitDirectMode() const {
    NJson::TJsonValue init;
    TString requestText;
    TString requestRegion;
    auto queryValue = Request[QueryColumn];

    if (queryValue.IsUndefined()) {
        Cerr << "found undefined value for query column " << QueryColumn;
    } else {
        requestText = queryValue.ConvertTo<TString>();
    }

    if (IsRegionValue) {
        requestRegion = RegionColumnOrValue;
    } else if (Request[RegionColumnOrValue].IsUndefined()) {
        requestRegion = "-1";
        Cerr << "found undefined value for region column " << RegionColumnOrValue;
    } else {
        requestRegion = Request[RegionColumnOrValue].ConvertTo<TString>();
    }

    THolder<TCgiParameters> cgi;
    const TString& req = "&text=" + CGIEscapeRet(requestText) + "&lr=" + requestRegion + "&" + AppendCgi;
    cgi.Reset(new TCgiParameters(req));
    cgi->ReplaceUnescaped("format", "json");
    return cgi;
}

void TProcessRowTask::AppendBegemotConfig(NJson::TJsonValue& ctx, const NJson::TJsonValue& begemotConfig) {
    NJson::TJsonValue config = begemotConfig;
    config[NAppHost::TYPE_FIELD] = "begemot_config";
    NJson::TJsonValue item = NJson::JSON_MAP;
    item["name"] = "BEGEMOT_CONFIG";
    NJson::TJsonValue results = NJson::JSON_ARRAY;
    results.AppendValue(config);
    item["results"] = results;
    ctx.AppendValue(item);
}

void TProcessRowTask::FillNormalParameters(TNode& result) {
    if (ShardName != "Merger") {
        FillShardParameters(result);
    }

    result["reqid"] = Request["reqid"];
    if (Request.HasKey("__query_hash__")) {
        result["__query_hash__"] = Request["__query_hash__"];
    }
}

void TProcessRowTask::FillShardParameters(TNode& result) {
    result["shard"] = ShardName;
    result["has-misspelling"] = Request.HasKey("has-misspelling") ? Request["has-misspelling"] : false;
    result["has-reask"] = Request.HasKey("has-reask") ? Request["has-reask"] : false;
    result["has-grunwald"] = Request.HasKey("has-grunwald") ? Request["has-grunwald"] : false;
    if (result["has-grunwald"].AsBool()) {
        result["begemot_grunwald_out"] = Request["begemot_grunwald_out"];
    }
}

void TProtobufAwareTestContext::AddProtobufItem(
    const google::protobuf::Message& item,
    TStringBuf type,
    const NAppHost::EContextItemKind kind
) {
    ProtobufTypes.insert(TString(type));
    TTestContext::AddProtobufItem(item, type, kind);
}

const NJson::TJsonValue& TProtobufAwareTestContext::GetResult() {
    if (ProtobufTypes.empty()) {
        return TTestContext::GetResult();
    }
    Result = TTestContext::GetResult();
    for (const auto& type : ProtobufTypes) {
        for (const auto& item : TTestContext::GetProtobufItemRefs(type, NAppHost::EContextItemSelection::Output)) {
            NJson::TJsonValue wrapper;
            wrapper[NAppHost::CONTENT_TYPE_FIELD] = "protobuf";
            wrapper[NAppHost::TYPE_FIELD] = type;
            wrapper[NAppHost::BINARY_FIELD] = Base64Encode(item.Raw());
            Result.AppendValue(wrapper);
        }
    }
    return Result;
}

TLogFrameVisitor::TLogFrameVisitor(TVector<TBegemotErrorEvent>& foundEvents)
    : FoundEvents(foundEvents)
{
    for (auto& cls : {"TRequestError", "TRuleError"}) {
        FilteredEvents.insert(NEvClass::Factory()->ClassByName(cls));
    }
}

void TLogFrameVisitor::Visit(const TEvent &event) {
    if (!FilteredEvents.contains(event.Class)) {
        return;
    }
    auto* message = event.Get<NProtoBuf::Message>();
    TBegemotErrorEvent& e = FoundEvents.emplace_back();
    e.type = event.GetName();
    if (auto* requestError = dynamic_cast<const NBg::NEv::TRequestError*>(message)) {
        e.code = requestError->Getcode();
    }
    if (auto* ruleError = dynamic_cast<const NBg::NEv::TRuleError*>(message)) {
        e.rule = ruleError->Getname();
    }
    TStringOutput out(e.text);
    SerializeToTextFormat(*message, out);
}

static const TString* GetLastParam(const TCgiParameters& cgi, TStringBuf name) {
    auto range = cgi.equal_range(name);
    return range.first != range.second ? &(--range.second)->second : nullptr;
}
