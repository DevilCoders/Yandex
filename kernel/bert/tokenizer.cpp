#include "tokenizer.h"

#include <dict/libs/trie/virtual_trie_iterator.h>

#include <util/stream/file.h>

using namespace NDict::NUtil;
using namespace NDict::NSegmenter;

namespace {
    struct TUtf16Transformer {
        using TResult = TUtf16String;
        TResult operator()(TUtf16String&& v) const noexcept {
            return v;
        }
    };

    struct TUtf32Transformer {
        using TResult = TUtf32String;
        TResult operator()(TUtf16String&& v) const noexcept {
            return TUtf32String::FromUtf16(v);
        }
    };
}

namespace NBertApplier {
    TBertSegmenter::TBertSegmenter(const TString& startTriePath, const TString& continuationTriePath)
        : StartTrie(WTrie::FromFile(startTriePath))
        , ContinuationTrie(WTrie::FromFile(continuationTriePath))
        , BaseSegmenter(CreateCharBasedSegmenter().release())
    {
    }

    TBertSegmenter::TBertSegmenter(IInputStream* startTrieStream, IInputStream* continuationTrieStream)
        : StartTrie(WTrie::FromStream(startTrieStream))
        , ContinuationTrie(WTrie::FromStream(continuationTrieStream))
        , BaseSegmenter(CreateCharBasedSegmenter().release())
    {
    }


    TBertIdConverter::TBertIdConverter(const TString& vocabPath) {
        int id = 0;
        TString token;

        TFileInput vocabFile(vocabPath);
        while (vocabFile.ReadLine(token)) {
            Token2Id[TUtf32String::FromUtf8(token)] = id++;
        }
        UnknownTokenId = Token2Id.at(TUtf32String::FromUtf8("[UNK]"));
    }

    TBertIdConverter::TBertIdConverter(IInputStream& vocabInStream) {
        int id = 0;
        TString token;

        while (vocabInStream.ReadLine(token)) {
            Token2Id[TUtf32String::FromUtf8(token)] = id++;
        }
        UnknownTokenId = Token2Id.at(TUtf32String::FromUtf8("[UNK]"));
    }

    size_t TBertSegmenter::Lookup(const TUtf16String* begin, const TUtf16String* end
                                    , const WTrie& currentTrie) const
    {
        auto trieIter = WVirtualTrieIterator(currentTrie.GetRoot());

        size_t matchLength = 0;
        size_t maxMatchLength = 0;

        while (begin != end) {
            if (!trieIter.Descend(begin++->data(), &currentTrie)) {
                break;
            }
            ++matchLength;
            if (trieIter.IsTerminal()) {
                maxMatchLength = matchLength;
            }
        }
        return Max<size_t>(1, maxMatchLength);
    }

    template<typename TFinalTransformer>
    void TBertSegmenter::TransformWord(const TUtf16String& word, TVector<typename TFinalTransformer::TResult>& result) const {
        const auto segments = BaseSegmenter->Split(word);
        auto begin = segments.begin();
        const auto end = segments.end();
        if (begin == end) {
            return;
        }

        auto matcher = [&result, &begin, end, this](NDict::NUtil::WTrie& trie, const TUtf16String& prefix) {
            const size_t maxMatchLength = Lookup(begin, end, trie);
            TUtf16String maxMatchSegment;
            for (size_t i = 0; i < maxMatchLength; ++i) {
                maxMatchSegment.append(*begin++);
            }
            result.push_back(TFinalTransformer()(prefix + maxMatchSegment));
        };

        matcher(*StartTrie, {});
        static const auto prefix = TUtf16String::FromUtf8("##");
        while (begin != end) {
            matcher(*ContinuationTrie, prefix);
        }
    }

    TVector<TUtf16String> TBertSegmenter::Split(const TUtf16String& text) const {
       return Split(text, std::numeric_limits<size_t>::max());
    }

    TVector<TUtf16String> TBertSegmenter::Split(TWtringBuf text, size_t maxResults) const {
        // assuming text is already normalized
        TVector<TUtf16String> result;
        while (result.size() < maxResults) {
            TWtringBuf word;
            if (!text.NextTok(u' ', word)) {
                break;
            }
            TransformWord<TUtf16Transformer>(TUtf16String(word), result);
        }
        result.crop(maxResults);
        return result;
    }

    TVector<TUtf32String> TBertSegmenter::Split(TUtf32StringBuf text, size_t maxResults) const {
        TVector<TUtf32String> result;
        while (result.size() < maxResults) {
            TUtf32StringBuf word;
            if (!text.NextTok(U' ', word)) {
                break;
            }
            const auto utf16Word = UTF32ToWide(word.data(), word.size());
            if (utf16Word.size() != word.size()) {
                result.push_back(TUtf32String(word));
            } else {
                TransformWord<TUtf32Transformer>(utf16Word, result);
            }
        }
        result.crop(maxResults);
        return result;
    }

    TVector<int> TBertIdConverter::Convert(const TVector<TUtf32String>& tokens) const {
        TVector<int> ids(Reserve(tokens.size()));
        for (const auto& token : tokens) {
            auto it = Token2Id.find(token);
            ids.push_back((it != Token2Id.end()) ? it->second : UnknownTokenId);
        }
        return ids;
    }
}
