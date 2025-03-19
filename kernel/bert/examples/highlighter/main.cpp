#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <util/system/info.h>
#include <util/stream/output.h>

#include "bert_highlighter.h"
#include "util.h"

struct TOptions {
    TString InputFile;
    TString Weights;
    TString StartTrie;
    TString ContTrie;
    TString Vocab;
    bool UseCpu;
    bool UseFp16;
    int DeviceIndex;
    size_t NumThreads;
    int MaxBatchSize;
    int MaxInputLength;
};

TOptions ParseOpts(int argc, const char** argv) {
    TOptions opts;
    auto srcOpts = NLastGetopt::TOpts::Default();
    srcOpts.AddLongOption("input")
        .StoreResult(&opts.InputFile)
        .Required()
        .Help("file with the input queries");
    srcOpts.AddLongOption("weights")
        .StoreResult(&opts.Weights)
        .Required()
        .Help("path to a file with the model weights");
    srcOpts.AddLongOption("starttrie")
        .StoreResult(&opts.StartTrie)
        .Required()
        .Help("path to a file with a start trie for tokenization");
    srcOpts.AddLongOption("conttrie")
        .StoreResult(&opts.ContTrie)
        .Required()
        .Help("path to a file with a cont trie for tokenization");
    srcOpts.AddLongOption("vocab")
        .StoreResult(&opts.Vocab)
        .Required()
        .Help("path to a file with a vocabulary");
    srcOpts.AddLongOption("cpu")
        .NoArgument()
        .Optional()
        .StoreValue(&opts.UseCpu, true)
        .DefaultValue(false);
    srcOpts.AddLongOption("fp16")
        .NoArgument()
        .Optional()
        .Help("load a model with FP16 weights (use dict/mt/make/tools/tfnn/convert_to_fp16 to convert an existing FP32 model)")
        .StoreValue(&opts.UseFp16, true)
        .DefaultValue(false);
    srcOpts.AddLongOption("device")
        .Help("when using GPU, selects the index of GPU to use")
        .Optional()
        .StoreResult(&opts.DeviceIndex)
        .DefaultValue(false);
    srcOpts.AddLongOption("threads")
        .Help("when using CPU, sets the thread count for OpenMP/MKL")
        .Optional()
        .StoreResult(&opts.NumThreads)
        .DefaultValue(static_cast<int>(NSystemInfo::NumberOfCpus()));
    srcOpts.AddLongOption("maxbatchsize")
        .StoreResult(&opts.MaxBatchSize)
        .Optional()
        .DefaultValue(32);
    srcOpts.AddLongOption("maxinputlength")
        .StoreResult(&opts.MaxInputLength)
        .Optional()
        .DefaultValue(127);

    NLastGetopt::TOptsParseResult res(&srcOpts, argc, argv);

    return opts;
}

template <typename TFloatType>
void RunAndReport(const TOptions& opts) {
    // Report in JSON to stdout.
    auto samples = NBertHighlighter::ReadSamples(opts.InputFile);
    auto [data, mapping] = NBertHighlighter::PrepareDataBatch(
        samples,
        opts.MaxInputLength,
        opts.StartTrie,
        opts.ContTrie,
        opts.Vocab
    );
    auto highlighter = MakeHolder<NBertHighlighter::TBertHighlighter<TFloatType>>(
        opts.MaxBatchSize,
        opts.MaxInputLength,
        opts.Weights,
        NBertHighlighter::CreateBackend(opts.UseCpu, opts.DeviceIndex, opts.NumThreads)
    );

    TConstArrayRef<TVector<int>> warmupBatch(data.begin(), data.begin() + Min<size_t>(data.size(), opts.MaxBatchSize));
    highlighter->ProcessBatch(warmupBatch);

    Cout << "[\n";
    NJson::TJsonWriterConfig jsonConfig;
    jsonConfig.WriteNanAsString = true;
    jsonConfig.FormatOutput = true;
    for (ui64 batchNo = 0U, batchCnt = (samples.size() - 1) / opts.MaxBatchSize + 1; batchNo < batchCnt; ++batchNo) {
        size_t start = batchNo * opts.MaxBatchSize;
        auto end = Min(start + opts.MaxBatchSize, data.size());
        Y_ENSURE(
            end - start <= static_cast<ui64>(opts.MaxBatchSize),
            "MaxBatchSize exceeded: got " << (end - start) << " with MaxBatchSize = " << opts.MaxBatchSize
        );

        TConstArrayRef<TVector<int>> batch(data.begin() + start, data.begin() + end);
        auto result = highlighter->ProcessBatch(batch);

        const size_t batchSize = batch.size();
        const size_t seqSizeAligned = result.size() / batchSize;
        const size_t classesNo = highlighter->GetOutputSize();
        for (ui64 seqNo = 0U; seqNo < batchSize; ++seqNo) {
            const size_t globalSeqNo = batchNo * batchSize + seqNo;
            NJson::TJsonValue jsonTokens(NJson::JSON_ARRAY);
            for (const int token : data[globalSeqNo])
                jsonTokens.AppendValue(token);

            NJson::TJsonValue jsonProbs(NJson::JSON_ARRAY);
            ui64 resultIdx = seqNo * seqSizeAligned + classesNo; // Ignore [CLS] token
            auto seqEnd = resultIdx + (data[globalSeqNo].size() + 1) * classesNo; // Ignore [CLS] token
            while (resultIdx < seqEnd) {
                NJson::TJsonValue classProbs(NJson::JSON_ARRAY);
                for (auto classIdx = resultIdx, classesEnd = resultIdx + classesNo; classIdx < classesEnd; ++classIdx) {
                    classProbs.AppendValue(static_cast<float>(result[classIdx]));
                }
                jsonProbs.AppendValue(classProbs);
                resultIdx += classesNo;
            }
            NJson::TJsonValue jsonMapping(NJson::JSON_ARRAY);
            for (const auto& [wordStart, wordLen] : mapping[globalSeqNo]) {
                NJson::TJsonValue coords(NJson::JSON_ARRAY);
                coords.AppendValue(wordStart);
                coords.AppendValue(wordLen);
                jsonMapping.AppendValue(coords);
            }

            NJson::TJsonValue record(NJson::JSON_MAP);
            record.InsertValue("text", samples[globalSeqNo]);
            record.InsertValue("tokens", std::move(jsonTokens));
            record.InsertValue("probs", std::move(jsonProbs));
            record.InsertValue("mapping", std::move(jsonMapping));
            NJson::WriteJson(&Cout, &record, jsonConfig);
            if (!(batchNo + 1 == batchCnt && seqNo + 1 == batchSize)) // don't put comma after last record
                Cout << ",";
            Cout << "\n";
        }
    }
    Cout << "]\n";
}

int main(int argc, const char** argv) {
    auto opts = ParseOpts(argc, argv);

    if (opts.UseFp16) {
        RunAndReport<TFloat16>(opts);
    } else {
        RunAndReport<float>(opts);
    }

    return 0;
}
