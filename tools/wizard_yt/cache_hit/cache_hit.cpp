#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/openssl/crypto/sha.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/logging/logger.h>

#include <search/wizard/face/rules_names.h>
#include <search/wizard/core/wizardcgi.h>

#include <util/string/split.h>
#include <library/cpp/cgiparam/cgiparam.h>

using namespace NYT;

namespace {
    class TCacheMapper
        : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
    public:
        void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
            for (; input->IsValid(); input->Next()) {
                TNode requestRow = input->GetRow();
                TCgiParameters cgiParams(requestRow["prepared_request"].AsString());
                const TString& eventData = requestRow["data"].AsString();
                const auto shasum = NOpenSsl::NSha256::Calc(eventData);
                requestRow["data_sha"] = TString(reinterpret_cast<const char*>(shasum.data()), shasum.size());
                for (size_t cgi = EWizardCgiParam::WCGI_TEXT; cgi != EWizardCgiParam::WCGI_MAX; ++cgi) {
                    EWizardCgiParam wizardCgi = static_cast<EWizardCgiParam>(cgi);
                    if (TWizardCgi::IsCacheKey(wizardCgi)) {
                        TString cgiName = TWizardCgi::NameByParam(wizardCgi);
                        if (cgiParams.Has(cgiName)) {
                            requestRow[cgiName] = cgiParams.Get(cgiName);
                        }
                    }
                }

                output->AddRow(requestRow);
            }
        }
    };

    REGISTER_MAPPER(TCacheMapper);

    class TCacheReducer
       : public IReducer<TTableReader<TNode>, TTableWriter<TNode>> {

    public:
        void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
            THashMap<TString, THashSet<TString>> distinctCgis;
            TNode outputRow;
            i64 count = 0;
            for (; input->IsValid(); input->Next()) {
                const TNode& row = input->GetRow();
                if (!count) {
                    outputRow = row;
                }
                for (size_t cgi = EWizardCgiParam::WCGI_TEXT; cgi != EWizardCgiParam::WCGI_MAX; ++cgi) {
                    EWizardCgiParam wizardCgi = static_cast<EWizardCgiParam>(cgi);
                    if (TWizardCgi::IsCacheKey(wizardCgi)) {
                        TString cgiName = TWizardCgi::NameByParam(wizardCgi);
                        if (row[cgiName].IsString()) {
                            distinctCgis[cgiName].insert(row[cgiName].AsString());
                        }
                    }
                }
                ++count;
            }
            auto counters = TNode::CreateMap();
            for (auto it = distinctCgis.begin(); it != distinctCgis.end(); ++it) {
                counters[it->first] = i64(it->second.size());
            }
            outputRow["cache_counters"] = counters;
            outputRow["cached_count"] = count;
            output->AddRow(outputRow);
        }

        TCacheReducer()
        {
        }
    };

    REGISTER_REDUCER(TCacheReducer);
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
        .AddLongOption('o', "output_table", "Path to output table")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputPath);

    TMaybe<TString> pool;
    options
        .AddLongOption('p', "pool", "YT pool")
        .Optional()
        .DefaultValue("search-runtime")
        .StoreResultT<TString>(&pool);

    TString proxy;
    options.
        AddLongOption('x', "proxy", "YT cluster")
        .Optional()
        .DefaultValue("freud")
        .StoreResult(&proxy);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    TNode commonSpec = TNode()("title", "run-cache-hit");
    if (pool.Defined()) {
        commonSpec("pool", *pool);
    }

    auto client = NYT::CreateClient(proxy);

    auto schemaNode = TNode::CreateList()
        .Add(TNode()("name", "ts")("type", "int64"))
        .Add(TNode()("name", "cached_count")("type", "int64"))
        .Add(TNode()("name", "frame_id")("type", "int64"))
        .Add(TNode()("name", "prepared_request")("type", "string"))
        .Add(TNode()("name", "uri_hash")("type", "string"))
        .Add(TNode()("name", "data")("type", "string"))
        .Add(TNode()("name", "processed_rules")("type", "any"))
        .Add(TNode()("name", "cache_counters")("type", "any"))
        .Add(TNode()("name", "data_sha")("type", "string"))
        .Add(TNode()("name", "cache_key")("type", "string"))
        .Add(TNode()("name", "event_type")("type", "string"))
        .Add(TNode()("name", "event_data")("type", "string"))
        .Add(TNode()("name", "instance")("type", "string"));

    for (size_t cgi = EWizardCgiParam::WCGI_TEXT; cgi != EWizardCgiParam::WCGI_MAX; ++cgi) {
        EWizardCgiParam wizardCgi = static_cast<EWizardCgiParam>(cgi);
        if (TWizardCgi::IsCacheKey(wizardCgi)) {
            TString cgiName = TWizardCgi::NameByParam(wizardCgi);
            schemaNode.Add(TNode()("name", cgiName)("type", "string"));
        }
    }

    client->Create(
        outputPath,
        NYT::NT_TABLE,
        TCreateOptions().Attributes(TNode()("schema", schemaNode))
    );

    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(TRichYPath(inputPathToLogTable))
            .AddOutput<TNode>(TRichYPath(outputPath)),
        new TCacheMapper(),
        TOperationOptions().Spec(commonSpec)
    );

    client->Sort(
        TSortOperationSpec()
            .AddInput(TRichYPath(outputPath))
            .Output(TRichYPath(outputPath))
            .SortBy({"data_sha"}),
        TOperationOptions().Spec(commonSpec)
    );

    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy("data_sha")
            .SortBy("data_sha")
            .AddInput<TNode>(TRichYPath(outputPath).SortedBy("data_sha"))
            .AddOutput<TNode>(TRichYPath(outputPath))
            .ReducerSpec(TUserJobSpec()),
        new TCacheReducer(),
        TOperationOptions().Spec(commonSpec));

    return 0;
}
