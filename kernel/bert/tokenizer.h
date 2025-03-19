#pragma once

#include <dict/libs/segmenter/segmenter.h>
#include <dict/libs/trie/trie.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <limits>

namespace NBertApplier {
    class TBertSegmenter final : public NDict::NSegmenter::ISegmenter {
    public:
        TBertSegmenter(const TString& startTriePath, const TString& continuationTriePath);
        TBertSegmenter(IInputStream* startTrieStream, IInputStream* continuationTrieStream);
        TVector<TUtf16String> Split(const TUtf16String& text) const override;

        TVector<TUtf16String> Split(TWtringBuf text, size_t maxResults) const;
        TVector<TUtf32String> Split(TUtf32StringBuf text, size_t maxResults = std::numeric_limits<size_t>::max()) const;

    private:
        size_t Lookup(const TUtf16String* begin, const TUtf16String* end
                        , const NDict::NUtil::WTrie& currentTrie) const;

        template<typename TFinalTransformer>
        void TransformWord(const TUtf16String& word, TVector<typename TFinalTransformer::TResult>& result) const;

    private:
        const THolder<NDict::NUtil::WTrie> StartTrie;
        const THolder<NDict::NUtil::WTrie> ContinuationTrie;
        const THolder<ISegmenter> BaseSegmenter;
    };

    class TBertIdConverter {
    public:
        TBertIdConverter(const TString& vocabPath);
        TBertIdConverter(IInputStream& vocabInStream);
        TVector<int> Convert(const TVector<TUtf32String>& tokens) const;

    private:
        THashMap<TUtf32String, int> Token2Id;
        int UnknownTokenId;
    };
}
