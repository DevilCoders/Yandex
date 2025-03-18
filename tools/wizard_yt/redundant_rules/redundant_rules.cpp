#include <library/cpp/getopt/last_getopt.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>

#include <search/wizard/rules/factory.h>

#include <util/string/split.h>

using namespace NYT;

namespace {
    class TEventLogMapper
        : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
    public:
        void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
            for (; input->IsValid(); input->Next()) {
                TNode requestRow = input->GetRow();
                if (requestRow["event_type"].AsString() == "ReqWizardProcessedRule") {
                    output->AddRow(requestRow);
                }
            }
        }
    };

    REGISTER_MAPPER(TEventLogMapper);

    class TRedundantRulesReducer
       : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {

    public:
        void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
            for (; input->IsValid(); input->Next()) {
                TNode resultRow = input->GetRow();
                if (resultRow["event_data"].AsString().find("\t1") != TString::npos) {
                    output->AddRow(resultRow);
                    break;
                }
            }
        }

        TRedundantRulesReducer()
        {
        }
    };

    REGISTER_REDUCER(TRedundantRulesReducer);
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

    TMaybe<TString> pool;
    options
        .AddLongOption('p', "pool", "Path to output table")
        .Optional()
        .DefaultValue("search-runtime")
        .StoreResultT<TString>(&pool);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    TNode commonSpec = TNode()("title", "run-redundant-rules");
    if (pool.Defined()) {
        commonSpec("pool", *pool);
    }

    auto client = NYT::CreateClient("banach");

    if (outputPath.EndsWith("/") && outputPath != TString("//") && outputPath.size() > 1) {
        outputPath = outputPath.substr(0, outputPath.size() - 1);
    }

    Y_VERIFY(client->Exists(outputPath));

    auto filteredEvents = outputPath + "/ruleEvents";
    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(TRichYPath(inputPathToLogTable))
            .AddOutput<TNode>(TRichYPath(filteredEvents)),
        new TEventLogMapper(),
        TOperationOptions().Spec(commonSpec)
    );

    if (pool.Defined()) {
        commonSpec("pool", *pool);
    }

    client->Sort(
        TSortOperationSpec()
            .AddInput(TRichYPath(filteredEvents))
            .Output(TRichYPath(filteredEvents))
            .SortBy({"event_data"}),
        TOperationOptions().Spec(commonSpec)
    );

    auto spec = TNode()
        ("title", "run-redundant-rules")
        ("reducer", TNode::CreateMap()
            ("tmpfs_path", ".")
            ("copy_files", true)
        );
    if (pool.Defined()) {
        spec("pool", *pool);
    }

    auto ruleEventsPath = outputPath + "/reducedRuleEvents";
    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy("event_data")
            .SortBy("event_data")
            .AddInput<TNode>(TRichYPath(filteredEvents).SortedBy("event_data"))
            .AddOutput<TNode>(TRichYPath(ruleEventsPath))
            .ReducerSpec(TUserJobSpec()),
        new TRedundantRulesReducer(),
        TOperationOptions().Spec(spec));

    auto reader = client->CreateTableReader<TNode>(ruleEventsPath);
    THashSet<TString> rulesNames;
    for (; reader->IsValid(); reader->Next()) {
        const auto& row = reader->GetRow();
        TVector<TString> ruleAndMarker;
        Split(row["event_data"].AsString(), "\t", ruleAndMarker);
        rulesNames.insert(ruleAndMarker[0]);
    }

    auto factory = NWiz::AllRules();
    auto writer = client->CreateTableWriter<TNode>(outputPath + "/unusedRules");
    for (const auto& rule : factory) {
        if (rulesNames.find(rule.Name)) {
            continue;
        } else {
            Cout << "Not found rule: " << rule.Name << Endl;
            writer->AddRow(TNode::CreateMap()("rule", rule.Name));
        }
    }
    writer->Finish();

    return 0;
}
