#include <util/generic/fwd.h>
#include <util/stream/fwd.h>

#include <kernel/bert/bert_wrapper.h>
#include <kernel/bert/tokenizer.h>

namespace NBertHighlighter {
    using TWord2Tokens = std::pair<size_t, size_t>;

    // Split input into tokens, return mapping word2tokens.
    // First vector contains all tokens.
    // Second vector contains pairs with [word_start, word_length] positions in tokens vector
    // // for each word get via "StringSplitter::Split(' ')" from util/string/split.h
    std::tuple<TVector<TUtf32String>, TVector<TWord2Tokens>> Tokenize(
        const TString& s,
        THolder<NBertApplier::TBertSegmenter>& segmenter
    );

    // samples -> tokens -> ids & word2ids mapping
    std::tuple<TVector<TVector<int>>, TVector<TVector<TWord2Tokens>>> PrepareDataBatch(
        const TVector<TString>& samples,
        size_t maxInputLength,
        const TString& startTrieFilename,
        const TString& contTrieFilename,
        const TString& vocabFilename
    );

    // Create backend for YNMT Bert.
    NDict::NMT::NYNMT::TBackendPtr CreateBackend(bool useCpu, int deviceIndex, size_t numThreads);

    // Read and strip data line-by-line.
    TVector<TString> ReadSamples(const TString& filename);
} // namespace NBertHighlighter
