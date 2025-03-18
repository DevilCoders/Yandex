#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>

//#include <search/wizard/face/rules_names.h>

#include <util/string/split.h>

using namespace NYT;

namespace {
    constexpr size_t GB = 1 << 30;
    class TSlowQueriesReducer
       : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {

    i64 delta;
    public:
        void Save(IOutputStream& stream) const override {
            ::Save(&stream, delta);
        }

        void Load(IInputStream& stream) override {
            ::Load(&stream, delta);
        }

        void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
            i64 begin = 0;
            size_t startCounter = 0;
            TVector<TNode> requestRows;
            i64 prevTs = 0;
            i64 maxDuration = 0;
            TNode slowestRule;
            for (; input->IsValid(); input->Next()) {
                TNode const& row = input->GetRow();
                i64 ts = row["ts"].AsInt64();
                TString const& eventType = row["event_type"].AsString();
                if (eventType.StartsWith("ReqWizardRequestReceived")) {
                    requestRows.push_back(row);
                    begin = ts;
                }
                if (eventType.StartsWith("ReqWizardStartProcessingRules")) {
                    ++startCounter;
                }
                if (eventType.StartsWith("ReqWizardProcessedRule")) {
                    i64 ruleDuration = ts - prevTs;
                    if ((ruleDuration > maxDuration) && prevTs) {
                        maxDuration = ruleDuration;
                        slowestRule = row;
                    }
                }
                if (eventType.StartsWith("EndOfFrame")) {
                    i64 end = ts;
                    Y_VERIFY(end >= begin);
                    i64 duration = end - begin;
                    if (duration > delta && startCounter == 1) {
                        for (auto& resultRow: requestRows) {
                            resultRow["duration"] = duration;
                            resultRow["slowest_rule_name"] = slowestRule["event_data"].AsString();
                            resultRow["slowest_rule_duration"] = maxDuration;
                            output->AddRow(resultRow);
                        }
                    }
                    requestRows.clear();
                    begin = 0;
                    startCounter = 0;
                }
                prevTs = ts;
            }
        }

        TSlowQueriesReducer()
        : delta(0)
        {
        }

        TSlowQueriesReducer(i64 delta)
        : delta(delta)
        {
        }
    };

    REGISTER_REDUCER(TSlowQueriesReducer);
}

int main(int argc, char *argv[]) {
    NYT::Initialize(argc, (const char **)argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(ILogger::INFO));

    NLastGetopt::TOpts options;

    TString inputPathToLogTable;
    options
        .AddLongOption('i', "input_path", "Path to input log table")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&inputPathToLogTable);

    TString outputPath;
    options
        .AddLongOption('o', "output_directory", "Path to output directory")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputPath);

    i64 delta;
    options
        .AddLongOption('d', "delta", "delta in microseconds")
        .Required()
        .RequiredArgument("DELTA")
        .StoreResult(&delta);

    TMaybe<TString> pool;
    options
        .AddLongOption('p', "pool", "Path to output table")
        .Optional()
        .DefaultValue("search-runtime")
        .StoreResultT<TString>(&pool);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    TNode commonSpec = TNode()("title", "run-slow-queries");
    if (pool.Defined()) {
        commonSpec("pool", *pool);
    }

    auto client = NYT::CreateClient("freud");

    if (outputPath.EndsWith("/") && outputPath != TString("//") && outputPath.size() > 1) {
        outputPath = outputPath.substr(0, outputPath.size() - 1);
    }

    Y_VERIFY(client->Exists(outputPath));

    auto sortedEvents = outputPath + "/sortedEvents";

    if (pool.Defined()) {
        commonSpec("pool", *pool);
    }

    client->Sort(
        TSortOperationSpec()
            .AddInput(TRichYPath(inputPathToLogTable))
            .Output(TRichYPath(sortedEvents))
            .SortBy({"instance", "frame_id", "ts"}),
        TOperationOptions().Spec(commonSpec)
    );

    auto spec = TNode()
        ("title", "run-slow-queries")
        ("reducer", TNode::CreateMap()
            ("tmpfs_path", ".")
            ("copy_files", true)
            ("tmpfs_size", 1 * GB)
            ("memory_limit", 2 * GB)
        );
    if (pool.Defined()) {
        spec("pool", *pool);
    }

    auto slowRequestsPath = outputPath + "/slowRequests";
    auto schemaNode = TNode::CreateList()
        .Add(TNode()("name", "ts")("type", "int64"))
        .Add(TNode()("name", "frame_id")("type", "int64"))
        .Add(TNode()("name", "duration")("type", "int64"))
        .Add(TNode()("name", "slowest_rule_duration")("type", "int64"))
        .Add(TNode()("name", "event_type")("type", "string"))
        .Add(TNode()("name", "event_data")("type", "string"))
        .Add(TNode()("name", "slowest_rule_name")("type", "string"))
        .Add(TNode()("name", "instance")("type", "string"));
    client->Create(
        slowRequestsPath,
        NYT::NT_TABLE,
        TCreateOptions().Attributes(TNode()("schema", schemaNode))
    );

    client->Sort(
        TSortOperationSpec()
            .AddInput(TRichYPath(slowRequestsPath))
            .Output(TRichYPath(slowRequestsPath))
            .SortBy({"instance", "frame_id", "ts"}),
        TOperationOptions().Spec(commonSpec)
    );

    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"instance", "frame_id"})
            .AddInput<TNode>(TRichYPath(sortedEvents).SortedBy({"instance", "frame_id", "ts"}))
            .AddOutput<TNode>(TRichYPath(slowRequestsPath).SortedBy({"instance", "frame_id", "ts"}))
            .ReducerSpec(TUserJobSpec()),
        new TSlowQueriesReducer(delta),
        TOperationOptions().Spec(spec));

    return 0;
}
