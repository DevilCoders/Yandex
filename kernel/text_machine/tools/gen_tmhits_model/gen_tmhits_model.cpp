#include <kernel/text_machine/util/hits_serializer.h>
#include <kernel/text_machine/util/hits_serializer_offroad.h>

#include <kernel/idx_proto/feature_pool.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>

#include <mapreduce/yt/interface/client.h>

#include <util/system/env.h>
#include <util/system/unaligned_mem.h>

using namespace NOffroad;
using namespace NYT;
using namespace NTextMachine;

template<typename HitWriter, typename AnnWriter>
static void ProcessRecord(HitWriter& hitWriter, AnnWriter& annWriter, TStringBuf data, ui64* nonOffroadSize)
{
    NFeaturePool::TLine line;
    Y_ENSURE(line.ParseFromArray(data.data(), data.size()), "invalid protopool line");
    Y_ENSURE(line.HasTMHits(), "no text machine hits in protopool");
    THitsDeserializer deserializer(&line.GetTMHits().GetHits());
    deserializer.Init();
    THashSet<EStreamType> usedStreams;
    TBlockHit hit;
    bool allWeightsZero = true;
    ui32 hitCount = 0;
    THashMap<std::tuple<EStreamType, ui32, ui32>, ui32> ann2idx;
    while (deserializer.NextBlockHit(hit)) {
        std::tuple<EStreamType, ui32, ui32> annKey(hit.StreamRef().Type, hit.AnnotationRef().BreakNumber, hit.AnnotationRef().StreamIndex);
        if (!ann2idx.contains(annKey)) {
            usedStreams.insert(hit.StreamRef().Type);
            ann2idx.emplace(annKey, ann2idx.size());
            TTextMachineAnnotationHit offroadAnn;
            THitsSerializer::ConvertToOffroad(hit.AnnotationRef(), offroadAnn);
            annWriter.WriteHit(offroadAnn);
            if (nonOffroadSize && (hit.AnnotationRef().Text || hit.AnnotationRef().Language)) {
                NTextMachineProtocol::TPbAnnotation protoAnn;
                protoAnn.SetText(hit.AnnotationRef().Text);
                protoAnn.SetLanguage(hit.AnnotationRef().Language);
                *nonOffroadSize += protoAnn.ByteSize();
            }
        }
        ++hitCount;
        if (hit.Weight != 0)
            allWeightsZero = false;
        TTextMachineHit offroadHit;
        THitsSerializer::ConvertToOffroad(hit, offroadHit, ann2idx[annKey]);
        hitWriter.WriteHit(offroadHit);
    }
    if (nonOffroadSize) {
        if (!allWeightsZero)
            *nonOffroadSize += hitCount * sizeof(float);
        if (usedStreams.size() > 1) {
            int streamBits = MostSignificantBit(usedStreams.size() - 1) + 1;
            *nonOffroadSize += (6 + streamBits * ann2idx.size() + 7) / 8;
        }
    }
}

class TCountingOutputStream : public IOutputStream
{
public:
    ui64 TotalWritten = 0;
    void DoWrite(const void*, size_t len) override {
        TotalWritten += len;
    }
};

static int GenerateModel(int argc, const char* argv[])
{
    TString srcTable, srcFile, hitModelFile, annModelFile;
    NLastGetopt::TOpts opts;
    opts.AddLongOption('t', "src-table", "source YT table with pool").RequiredArgument("TABLE").StoreResult(&srcTable);
    opts.AddLongOption('f', "src-file", "source file with pool in final format").RequiredArgument("FILE").StoreResult(&srcFile);
    opts.AddLongOption("dst-hit-model", "target file with hit model").Required().RequiredArgument("FILE").StoreResult(&hitModelFile);
    opts.AddLongOption("dst-ann-model", "target file with annotation model").Required().RequiredArgument("FILE").StoreResult(&annModelFile);
    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult cmdLine(&opts, argc, argv);

    if (!srcTable && !srcFile || srcTable && srcFile) {
        Cerr << "exactly one of --src-table and --src-file must be specified\n";
        return 1;
    }

    // static to avoid stack overflow
    static TTupleSampler<TTextMachineHit, TTextMachineHitVectorizer, TTextMachineHitSubtractor, TSampler64, PlainOldBuffer> sampler1;
    static TTupleSampler<TTextMachineAnnotationHit, TTextMachineAnnotationHitVectorizer, TTextMachineAnnotationHitSubtractor, TSampler64, PlainOldBuffer> sampler2;

    if (srcFile) {
        TVector<char> data;
        TFileInput in(srcFile);
        ui32 recordSize;
        while (in.Load(&recordSize, sizeof(ui32)) == sizeof(ui32)) {
            data.resize(recordSize);
            Y_ENSURE(in.Load(data.data(), recordSize) == recordSize);
            ProcessRecord(sampler1, sampler2, TStringBuf(data.data(), data.size()), nullptr);
            sampler1.FinishBlock();
            sampler2.FinishBlock();
        }
    }
    if (srcTable) {
        Initialize(argc, argv);
        auto client = CreateClient(GetEnv("YT_PROXY"));
        auto reader = client->CreateTableReader<TYaMRRow>(srcTable);
        for (; reader->IsValid(); reader->Next()) {
            TStringBuf val = reader->GetRow().Value;
            Y_ENSURE(val.Size() >= sizeof(ui32), "invalid protopool record size");
            Y_ENSURE(val.Size() == sizeof(ui32) + ReadUnaligned<ui32>(val.data()), "invalid protopool record size");
            ProcessRecord(sampler1, sampler2, val.Skip(sizeof(ui32)), nullptr);
            sampler1.FinishBlock();
            sampler2.FinishBlock();
        }
    }

    auto model1 = sampler1.Finish();
    auto model2 = sampler2.Finish();
    model1.Save(hitModelFile);
    model2.Save(annModelFile);
    return 0;
}

static int CalcCompressedSize(int argc, const char* argv[])
{
    TString srcTable, srcFile, hitModelFile, annModelFile;
    NLastGetopt::TOpts opts;
    opts.AddLongOption('t', "src-table", "source YT table with pool").RequiredArgument("TABLE").StoreResult(&srcTable);
    opts.AddLongOption('f', "src-file", "source file with pool in final format").RequiredArgument("FILE").StoreResult(&srcFile);
    opts.AddLongOption("src-hit-model", "source file with hit model").RequiredArgument("FILE").StoreResult(&hitModelFile);
    opts.AddLongOption("src-ann-model", "source file with annotation model").RequiredArgument("FILE").StoreResult(&annModelFile);
    opts.SetFreeArgsNum(0);
    NLastGetopt::TOptsParseResult cmdLine(&opts, argc, argv);

    static TTextMachineHitWriter::TModel model1;
    model1.Load(hitModelFile);
    static TTextMachineHitWriter::TTable table1(model1);
    static TTextMachineHitWriter writer1;

    static TTextMachineAnnotationWriter::TModel model2;
    model2.Load(annModelFile);
    static TTextMachineAnnotationWriter::TTable table2(model2);
    static TTextMachineAnnotationWriter writer2;

    TCountingOutputStream out;
    ui64 nonOffroadSize = 0;
    if (srcFile) {
        TVector<char> data;
        TFileInput in(srcFile);
        ui32 recordSize;
        while (in.Load(&recordSize, sizeof(ui32)) == sizeof(ui32)) {
            data.resize(recordSize);
            Y_ENSURE(in.Load(data.data(), recordSize) == recordSize);
            writer1.Reset(&table1, &out);
            writer2.Reset(&table2, &out);
            ProcessRecord(writer1, writer2, TStringBuf(data.data(), data.size()), &nonOffroadSize);
            writer1.Finish();
            writer2.Finish();
        }
    }
    if (srcTable) {
        auto client = CreateClient(GetEnv("YT_PROXY"));
        auto reader = client->CreateTableReader<TYaMRRow>(srcTable);
        for (; reader->IsValid(); reader->Next()) {
            TStringBuf val = reader->GetRow().Value;
            Y_ENSURE(val.Size() >= sizeof(ui32), "invalid protopool record size");
            Y_ENSURE(val.Size() == sizeof(ui32) + ReadUnaligned<ui32>(val.data()), "invalid protopool record size");
            writer1.Reset(&table1, &out);
            writer2.Reset(&table2, &out);
            ProcessRecord(writer1, writer2, val.Skip(sizeof(ui32)), &nonOffroadSize);
            writer1.Finish();
            writer2.Finish();
        }
    }
    Cout << "total written: " << (out.TotalWritten + nonOffroadSize) << " = " << out.TotalWritten << " + " << nonOffroadSize << '\n';
    return 0;
}

int main(int argc, const char* argv[])
{
    TModChooser chooser;
    chooser.AddMode("generate-model", &GenerateModel, "generate offroad models");
    chooser.AddMode("calc-compressed-size", &CalcCompressedSize, "calculate size of data compressed by given offroad models");
    return chooser.Run(argc, argv);
}
