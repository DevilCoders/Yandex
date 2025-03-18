#include <tools/wizard_yt/reducer_common/common.h>

#include <search/wizard/core/wizardcgi.h>

#include <library/cpp/openssl/crypto/sha.h>

#include <util/system/env.h>
#include <util/generic/guid.h>
#include <library/cpp/cgiparam/cgiparam.h>


class TEventLogMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    THashSet<TString> filterInstances;
public:
    void Save(IOutputStream& stream) const override {
        ::Save(&stream, filterInstances.size());
        for (const auto& instance: filterInstances) {
            ::Save(&stream, instance);
        }
    }

    void Load(IInputStream& stream) override {
        size_t filterInstancesSize = 0;
        ::Load(&stream, filterInstancesSize);
        for (size_t i = 0; i < filterInstancesSize; ++i) {
            TString instance;
            ::Load(&stream, instance);
            filterInstances.insert(instance);
        }
    }

    TEventLogMapper()
        : filterInstances()
    {
    }

    TEventLogMapper(const THashSet<TString>& filterInstances)
        : filterInstances(filterInstances)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TVector<TString> uriAndHeaders;
        for (; input->IsValid(); input->Next()) {
            const NYT::TNode& requestRow = input->GetRow();
            auto shortInstance = requestRow["instance"].AsString();
            auto delimiterPos = shortInstance.find_first_of(':');
            if (delimiterPos != TString::npos) {
                shortInstance = shortInstance.substr(0, delimiterPos);
            }
            if (filterInstances.size() && !filterInstances.contains(shortInstance)) {
                continue;
            }

            if (requestRow["event_type"].AsString() == "ReqWizardRequestReceived") {
                uriAndHeaders.clear();
                Split(requestRow["event_data"].AsString(), "\t", uriAndHeaders);
                if (uriAndHeaders.size() < 3) {
                    continue;
                }
                auto& requestData = uriAndHeaders[0];
                auto& requestType = uriAndHeaders[2];
                if ((requestType.StartsWith("cgi") && !requestData.StartsWith("action")) || requestType.StartsWith("apphost parsed")) {
                    NYT::TNode resultRow = requestRow;
                    TCgiParameters params(requestData);
                    NOpenSsl::NSha256::TCalcer sha;
                    for (size_t cgi = EWizardCgiParam::WCGI_TEXT; cgi != EWizardCgiParam::WCGI_MAX; ++cgi) {
                        EWizardCgiParam wizardCgi = static_cast<EWizardCgiParam>(cgi);
                        if (TWizardCgi::IsCacheKey(wizardCgi)) {
                            TString cgiName = TWizardCgi::NameByParam(wizardCgi);
                            if (auto range = params.Range(cgiName); !range.empty()) {
                                sha.Update(*std::prev(range.end()));
                            }
                        }
                    }
                    const auto shasum = sha.Final();

                    resultRow["uri_hash"] = TString(reinterpret_cast<const char*>(shasum.data()), shasum.size());
                    resultRow["prepared_request"] = requestData;

                    output->AddRow(resultRow);
                }
            }
        }
    }
};

REGISTER_MAPPER(TEventLogMapper);

int main(int argc, char *argv[]) {
    NYT::Initialize(argc, (const char **)argv);
    NYT::SetLogger(NYT::CreateStdErrLogger(NYT::ILogger::INFO));

    NLastGetopt::TOpts options;

    TString inputTable;
    options
        .AddLongOption('i', "input", "Path to input table")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&inputTable);

    TString outputTable;
    options
        .AddLongOption('o', "output", "Path to output table")
        .Required()
        .RequiredArgument("OUTPUT")
        .StoreResult(&outputTable);

    TVector<TString> filterInstances;
    options
        .AddLongOption('f', "filter_instance", "Run only requests from this instances")
        .AppendTo(&filterInstances)
        .Optional();

    bool simpleMap;
    options
        .AddLongOption('s', "simple_map", "Build cgi requests from columns")
        .SetFlag(&simpleMap);

    options.AddHelpOption('h');
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);


    NYT::TNode spec = NYT::TNode()("title", "Wizard eventlog mapper");
    TString pool = GetEnv("YT_POOL");
    if (!pool.empty()) {
        spec("pool", pool);
    }

    NYT::IClientPtr client = NYT::CreateClient(GetEnv("YT_PROXY"));
    THashSet<TString> filterInstancesSet;
    for (const auto& instance: filterInstances) {
        filterInstancesSet.insert(instance);
    }
    NYT::IMapperBase* mapper;
    if (simpleMap) {
        mapper = new TSimpleLogMapper();
    } else {
        mapper = new TEventLogMapper(filterInstancesSet);
    }
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath(inputTable))
            .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable)),
        mapper,
        NYT::TOperationOptions().Spec(spec)
    );
    return 0;
}
