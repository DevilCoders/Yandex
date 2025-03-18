#include <tools/wizard_yt/reducer_common/common.h>
#include <mapreduce/yt/interface/logging/logger.h>
#include <mapreduce/yt/interface/client.h>

#include <apphost/lib/converter/converter.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <util/generic/hash_set.h>
#include <util/digest/city.h>
#include <util/string/split.h>
#include <util/system/env.h>

using namespace NYT;

class TBegemotEventLogMapper : public IMapper<TTableReader<TNode>, TTableWriter<TNode>> {
private:
    THolder<NAppHost::NConverter::IConverter> Converter;
public:
    TBegemotEventLogMapper() {
    }

    void Start(TTableWriter<TNode>* ) override {
        NAppHost::NConverter::TConverterFactory converterFactory;
        Converter = converterFactory.Create("service_request");
    }

    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        Y_VERIFY(Converter);
        for (; input->IsValid(); input->Next()) {
            const auto& requestRow = input->GetRow();
            TNode resultRow = TNode::CreateMap();

            if (requestRow["event_type"].AsString() != "TRequestReceived") {
                continue;
            }

            auto shortInstance = requestRow["instance"].AsString();
            auto delimiterPos = shortInstance.find_first_of(':');
            if (delimiterPos != TString::npos) {
                shortInstance = shortInstance.substr(0, delimiterPos);
            }
            TString data = requestRow["event_data"].AsString();
            TVector<TString> parts;
            if (Split(data, "\t\n", parts) != 1) {
                continue;
            }
            TString base64Encoded = parts[0];
            TString rawRequestData = Base64Decode(base64Encoded);
            TString json = Converter->ConvertToJSON(rawRequestData, false);
            NJson::TJsonValue tmp;
            NJson::ReadJsonFastTree(TStringBuf(json), &tmp);
            auto& answers = tmp["answers"].GetArray();
            TString reqid, begemotGrunwaldOut;
            bool hasMisspelling = false, hasReask = false, hasGrunwald = false;
            for (const auto& item: answers) {
                TString name = item["name"].GetStringSafe();
                if (name == "INIT" && item.Has("results") && item["results"].IsArray()) {
                    for (const auto& res : item["results"].GetArraySafe()) {
                        if (res.Has("reqid") && res["reqid"].IsString()) {
                            reqid = res["reqid"].GetString();
                        }
                    }
                } else if ((name == "APP_HOST_PARAMS") && item.Has("results") && item["results"].IsArray()) {
                    for (const auto& res : item["results"].GetArraySafe()) {
                        if (res.Has("binary") && res["binary"].Has("reqid") && res["binary"]["reqid"].IsString()) {
                            reqid = res["binary"]["reqid"].GetString();
                        }
                    }
                } else if (name == "MISSPELL" && item.Has("meta")) {
                    for (const auto& flag : item["meta"].GetArraySafe()) {
                        if (flag == "has-misspelling") {
                            hasMisspelling = true;
                        } else if (flag == "has-reask") {
                            hasReask = true;
                        }
                    }
                } else if (name == "SKR" || name == "MISSPELL_POST_SETUP" || name == "INPUT") { // apphost nodes for megamind
                    if (auto ptr = item.GetValueByPath("results.[0].params.reqid.[0]")) {
                        reqid = ptr->GetString();
                        break;
                    }
                }
            }
            resultRow["has-reask"] = hasReask;
            resultRow["has-misspelling"] = hasMisspelling;
            resultRow["has-grunwald"] = hasGrunwald;
            resultRow["begemot_grunwald_out"] = begemotGrunwaldOut;
            resultRow["prepared_request"] = ToString(tmp["answers"]);
            resultRow["reqid"] = reqid;
            output->AddRow(resultRow, 0);
        }
    }
};

REGISTER_MAPPER(TBegemotEventLogMapper);

class TRepeatedRequestsFilter : public NYT::IReducer<TTableReader<TNode>, TTableWriter<TNode>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        auto& row = reader->GetRow();
        writer->AddRow(row);
    }
};
REGISTER_REDUCER(TRepeatedRequestsFilter);

int main(int argc, char *argv[]) {
    NYT::Initialize(argc, (const char **)argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(ILogger::INFO));

    NLastGetopt::TOpts options;
    TString inputTable;
    options
        .AddLongOption('i', "input", "Path to input eventlog table")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&inputTable);

    TString outputTable;
    options
        .AddLongOption('o', "output", "Path to output table")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputTable);

    bool cgi;
    options
        .AddLongOption('c', "cgi", "Build cgi requests from columns")
        .NoArgument()
        .SetFlag(&cgi);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    IClientPtr client = NYT::CreateClient(GetEnv("YT_PROXY"));
    TString title = cgi ? "Begemot CGI mapper" : "Begemot eventlog mapper";
    TNode spec = TNode()("title", title);
    TString pool = GetEnv("YT_POOL");
    if (!pool.empty()) {
        spec("pool", pool);
    }

    if (cgi) {
        client->Map(
            TMapOperationSpec()
                .AddInput<TNode>(inputTable)
                .AddOutput<TNode>(outputTable),
            new TSimpleLogMapper(),
            TOperationOptions().Spec(spec)
        );
    } else {
        client->MapReduce(
            TMapReduceOperationSpec()
                .AddInput<TNode>(inputTable)
                .AddOutput<TNode>(outputTable)
                .ReduceBy({"reqid", "has-grunwald", "has-misspelling"}),
            new TBegemotEventLogMapper(),
            new TRepeatedRequestsFilter(),
            TOperationOptions().Spec(spec)
        );
    }

    return 0;
}
