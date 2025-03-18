#include <tools/wizard_yt/reducer_common/common.h>

#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/interface/client.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/fast_sax/parser.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <util/system/env.h>
#include <util/string/split.h>


TVector<TString> SHARDS_ORDER = {"Wizard", "Ethos", "MisspellFeatures", "MisspellFeatures2", "Bravo", "Knn"};

class TMergeReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TMergeReducer() {
    }

    // input contains BEGEMOT_WORKERS, BEGEMOT_WORKERS_MISSPELL and BEGEMOT_WORKERS_GRUNWALD responses
    // produces input for BEGEMOT_MERGER, or BEGEMOT_MERGER_OPTIMISTIC, if no MISSPELL[has-misspelling]
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        try {
            THashMap<TString, NJson::TJsonValue> workersMisspell, workersGrunwald, workers;
            NJson::TJsonValue ctx;
            TString reqid;
            bool hasReask = false;
            TMaybe<ui64> query_hash;
            NJson::TJsonValue begemotGrunwald, begemotGrunwaldOut;
            for (; input->IsValid(); input->Next()) {
                NYT::TNode row = input->GetRow();
                TString sourceName = row["shard"].AsString();
                hasReask |= row["has-reask"].AsBool();
                NJson::TJsonValue answer;
                ReadJsonTree(row["begemot_answer"].AsString(), &answer);
                NJson::TJsonValue sourceResponse = NJson::JSON_MAP;
                sourceResponse["name"] = sourceName;
                sourceResponse["results"] = answer;
                if (row["has-grunwald"].AsBool()) {
                    workersGrunwald[sourceName] = std::move(sourceResponse);
                    if (!begemotGrunwaldOut.IsDefined()) {
                        ReadJsonFastTree(row["begemot_grunwald_out"].AsString(), &begemotGrunwaldOut);
                        begemotGrunwald = begemotGrunwaldOut;
                        ChangeType(begemotGrunwald);
                    }
                } else if (row["has-misspelling"].AsBool()) {
                    // if this branch won't be executed, output will be the same as BEGEMOT_MERGER_OPTIMISTIC receives
                    workersMisspell[sourceName] = std::move(sourceResponse);
                } else {
                    workers[sourceName] = std::move(sourceResponse);
                }
                NJson::TJsonValue request = NJson::JSON_ARRAY;
                reqid = row["reqid"].AsString();
                if (row.HasKey("__query_hash__") && !row["__query_hash__"].IsEntity()) {
                    query_hash = row["__query_hash__"].AsUint64();
                }
            }
            if (!hasReask) {
                for (auto& shard : SHARDS_ORDER) {
                    if (workers.contains(shard)) {
                        ctx.AppendValue(std::move(workers[shard]));
                    }
                }
            }
            for (auto& shard : SHARDS_ORDER) {
                if (workersMisspell.contains(shard)) {
                    ctx.AppendValue(std::move(workersMisspell[shard]));
                }
            }
            if (begemotGrunwaldOut.IsDefined()) {
                ctx.AppendValue(std::move(begemotGrunwald));
                ctx.AppendValue(std::move(begemotGrunwaldOut));
                for (auto& shard : SHARDS_ORDER) {
                    if (workersGrunwald.contains(shard)) {
                        ctx.AppendValue(std::move(workersGrunwald[shard]));
                    }
                }
            }
            if (!workersMisspell.empty() && workers.empty()) {
                return; // BEGEMOT_WORKERS log does not contain this request, skipping
            }
            NJson::TJsonValue& config = ctx.AppendValue(NJson::JSON_MAP);
            config["name"] = "BEGEMOT_CONFIG";
            NJson::TJsonValue& result = config["results"].AppendValue(NJson::TJsonValue());
            result["type"] = "begemot_config";
            result["merge"].AppendValue("wizard_grunwald");
            result["merge"].AppendValue("wizard_misspell");
            result["result_type"] = "wizard";
            NYT::TNode resultRow = NYT::TNode()("prepared_request", ToString(ctx))("reqid", reqid);
            if (query_hash.Defined()) {
                resultRow["__query_hash__"] = *query_hash;
            }
            output->AddRow(resultRow, 0);
        } catch (const TWithBackTrace<yexception>& e) {
            e.BackTrace()->PrintTo(Cerr);
            throw e;
        }
    }
private:
    // BEGEMOT_MERGER needs a response of BEGEMOT_GRUNWALD, but BEGEMOT_WORKERS logs contain
    // only BEGEMOT_GRUNWALD_OUT, which contains only an element of type wizard#GrunwaldPre from BEGEMOT_GRUNWALD
    // under type wizard_grunwald#GrunwaldPre. Merger uses only GrunwaldPre element from BEGEMOT_GRUNWALD,
    // and hence BEGEMOT_GRUNWALD can be created from BEGEMOT_GRUNWALD_OUT by changing type to wizard#GrunwaldPre
    void ChangeType(NJson::TJsonValue &begemotGrunwaldOut) {
        auto& results = begemotGrunwaldOut["results"];
        size_t n = results.GetArray().size();
        for (size_t i = 0; i < n; i++) {
            auto& type = results[i][NAppHost::TYPE_FIELD].GetString();
            Y_VERIFY(type.StartsWith("wizard_grunwald#"));
            results[i][NAppHost::TYPE_FIELD] = "wizard" + type.substr(strlen("wizard_grunwald"));
        }
    }
};

REGISTER_REDUCER(TMergeReducer);

int main(int argc, const char **argv) {
    NYT::Initialize(argc, argv);
    SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));

    NLastGetopt::TOpts options;
    TVector<TString> inputTables;
    options
        .AddLongOption('i', "input", "List of input tables, containing begemot workers answers")
        .AppendTo(&inputTables)
        .Optional();

    TString output;
    options
        .AddLongOption('o', "output", "Output table")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&output);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    NYT::IClientPtr client = NYT::CreateClient(GetEnv("YT_PROXY"));
    TString coreTablePath = output + ".core";
    client->Create(coreTablePath, NYT::NT_TABLE, NYT::TCreateOptions().Force(true).Recursive(true));

    NYT::TRichYPath outputTable = NYT::TRichYPath(output).Schema(REQUESTS_SCHEMA);
    auto spec = NYT::TMapReduceOperationSpec()
        .ReduceBy("reqid")
        .AddOutput<NYT::TNode>(outputTable)
        .MaxFailedJobCount(0)
        .CoreTablePath(coreTablePath);
    for (auto& table : inputTables) {
        spec.AddInput<NYT::TNode>(table);
    }

    client->MapReduce(
        spec,
        nullptr,
        new TMergeReducer(),
        NYT::TOperationOptions().Spec(NYT::TNode()("title", "Merging begemot workers answers"))
    );

    return 0;
}
