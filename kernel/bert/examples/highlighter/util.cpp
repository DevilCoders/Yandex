#include <kernel/bert/tokenizer.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/strip.h>

#include <dict/mt/libs/nn/ynmt_backend/cpu/backend.h>
#include <dict/mt/libs/nn/ynmt_backend/gpu_if_supported/backend.h>

#include "util.h"

namespace NBertHighlighter {
    std::tuple<TVector<TUtf32String>, TVector<TWord2Tokens>> Tokenize(
        const TString& s,
        THolder<NBertApplier::TBertSegmenter>& segmenter) {
        TVector<TUtf32String> tokens;
        TVector<TWord2Tokens> mapping;

        for (TStringBuf part : StringSplitter(s).Split(' ').SkipEmpty()) {
            TUtf32String w32Sentence = TUtf32String::FromUtf8(TString(part));
            auto partTokens = segmenter->Split(w32Sentence);
            mapping.push_back(std::make_pair(tokens.size(), partTokens.size()));
            tokens.insert(tokens.end(), partTokens.begin(), partTokens.end());
        }

        return std::make_tuple(tokens, mapping);
    }

    std::tuple<TVector<TVector<int>>, TVector<TVector<TWord2Tokens>>> PrepareDataBatch(
        const TVector<TString>& samples,
        size_t maxInputLength,
        const TString& startTrieFilename,
        const TString& contTrieFilename,
        const TString& vocabFilename) {
        TVector<TVector<int>> ids(Reserve(samples.size()));
        TVector<TVector<TWord2Tokens>> mappings(Reserve(samples.size()));

        THolder<NBertApplier::TBertSegmenter> segmenter(new NBertApplier::TBertSegmenter(startTrieFilename, contTrieFilename));
        THolder<NBertApplier::TBertIdConverter> converter(new NBertApplier::TBertIdConverter(vocabFilename));

        for (auto& sample : samples) {
            auto [tokens, mapping] = Tokenize(sample, segmenter);
            tokens.resize(Min(tokens.size(), maxInputLength - 2));
            auto mappingSize = mapping.size();
            while (mapping[mappingSize - 1].first >= tokens.size())
                --mappingSize;
            mapping.resize(mappingSize);
            if (mapping.back().first + mapping.back().second > tokens.size())
                mapping.back().second = tokens.size() - mapping.back().first;

            auto tokenIds = converter->Convert(tokens);
            ids.emplace_back(tokenIds.begin(), tokenIds.end());
            mappings.emplace_back(mapping.begin(), mapping.end());
        }

        return make_tuple(ids, mappings);
    }

    NDict::NMT::NYNMT::TBackendPtr CreateBackend(bool useCpu, int deviceIndex, size_t numThreads) {
        if (useCpu) {
            return NDict::NMT::NYNMT::CreateCpuBackend(numThreads);
        }
        auto result = NDict::NMT::NYNMT::CreateGpuBackendIfSupported(deviceIndex);
        Y_ENSURE(result, "No GPU backend available, use cpu backend.");
        return *result;
    }

    TVector<TString> ReadSamples(const TString& filename) {
        TIFStream file(filename);
        TVector<TString> result;
        TString str;
        while (file.ReadLine(str))
            result.push_back(StripString(str));
        return result;
    }
} // namespace NBertHighlighter
