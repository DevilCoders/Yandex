#include <mapreduce/yt/interface/client.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <util/stream/output.h>
#include <util/system/user.h>

#include <algorithm>

using namespace NYT;

class THeaderMapper
    : public IMapper<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    virtual void Do(TReader* reader, TWriter* writer) override
    {
        Y_UNUSED(writer);
        for (const auto& cursor : *reader) {
            const TNode& row = cursor.GetRow();
            const TString request = row["Data"].AsString();
            TStringInput stringInput{request};
            THttpInput input{&stringInput};
            const THttpHeaders headers = input.Headers();
            for(const THttpInputHeader& header : headers) {
                ++Frequency[header.Name()];
            }
        }
    }
    virtual void Finish(TWriter* writer) override {
        for(const auto& [header, freq] : Frequency) {
            TNode outRow;
            outRow["header"] = header;
            outRow["frequency"] = freq;
            writer->AddRow(outRow);
        }
    }
    THashMap<TString, size_t> Frequency;
};

class TFilterDuplicatesReduce
    : public IReducer<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    void Do(TReader* reader, TWriter* writer) override {
        size_t freq = 0;
        Y_UNUSED(writer);
        for (const auto& cursor : *reader) {
            const TNode& row = cursor.GetRow();
            Header = row["header"].AsString();
            freq += row["frequency"].AsUint64();
        }
        TNode outRow;
        outRow["header"] = Header;
        outRow["frequency"] = freq;
        writer->AddRow(outRow);
    }
    TString Header;
};

REGISTER_REDUCER(TFilterDuplicatesReduce);
REGISTER_MAPPER(THeaderMapper);

struct TConfig {
    TConfig(NLastGetopt::TOptsParseResult& options)
        : Client(NYT::CreateClient(options.Get("proxy")))
        , inputFolderPath(options.Get("source"))
        , outputTableRaw(options.Get("collect"))
        , outputTableSorted(options.Get("sort"))
        , outputTableUnique(options.Get("unique"))
    {}

    NYT::IClientPtr Client;
    
    TString inputFolderPath;
    TString outputTableRaw;
    TString outputTableSorted;
    TString outputTableUnique;
};

NLastGetopt::TOpts GetOptions()
{
    NLastGetopt::TOpts opts;

    opts.SetTitle("Header names parser");

    opts.AddLongOption("proxy", "YT cluster.")
        .DefaultValue("hahn");
    opts.AddLongOption("source", "YT source node with tables.")
        .DefaultValue("//home/antirobot/evlog_split/request_data/1d");
    opts.AddLongOption("collect", "YT destination table with actualized data.")
        .DefaultValue("//tmp/" + GetUsername() + "_actualized");
    opts.AddLongOption("sort", "YT destination table with sorted data.")
        .DefaultValue("//tmp/" + GetUsername() + "_sorted");
    opts.AddLongOption("unique", "YT destination table with unique header names.")
        .DefaultValue("//tmp/" + GetUsername() + "_unique");

    return opts;
}

int Run(int argc, const char** argv) {
    auto opts = GetOptions();
    NLastGetopt::TOptsParseResult options(&opts, argc, argv);
    const TConfig config(options);

    auto client = config.Client->StartTransaction();

    const auto listing = client->List(config.inputFolderPath);
    auto operationSpec = TMapOperationSpec();
    for(const TNode& path : listing) {
        operationSpec.AddInput<TNode>(config.inputFolderPath + "/" + path.AsString());
    }
    operationSpec.AddOutput<TNode>(config.outputTableRaw);

    auto op = client->Map(operationSpec, new THeaderMapper);

    client->Sort(
        TSortOperationSpec()
            .AddInput(config.outputTableRaw)
            .Output(config.outputTableSorted)
            .SortBy({"header"}));

    client->Reduce(
        TReduceOperationSpec()
            .ReduceBy({"header"})
            .AddInput<TNode>(config.outputTableSorted)
            .AddOutput<TNode>(config.outputTableUnique),
        new TFilterDuplicatesReduce);

    client->Sort(
        TSortOperationSpec()
            .AddInput(config.outputTableUnique)
            .Output(config.outputTableSorted)
            .SortBy({"frequency"}));
            
    client->Commit();
    return 0;
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);
    TModChooser mods;
    mods.AddMode("run", &Run, "Get unique headers and frequencies");
    return mods.Run(argc, argv);
}
