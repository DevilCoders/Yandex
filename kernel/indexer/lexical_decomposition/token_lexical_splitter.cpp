#include <library/cpp/on_disk/aho_corasick/reader.h>
#include <library/cpp/containers/comptrie/comptrie_trie.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>

#include "token_lexical_splitter.h"

using namespace NLexicalDecomposition;

class TSpecifiedVocabulary {
public:
    TSpecifiedVocabulary(const TBlob& blob)
        : Word2Id(GetBlock(blob, 0))
        , Id2Info(GetBlock(blob, 1))
        , AhoSearcher(GetBlock(blob, 2))
    {
    }

    const TDecompositionResultDescr CreateSingleWordDescr(const TUtf16String& word) const {
        ui32 id;
        if (Word2Id.Find(word.data(), word.size(), &id))
            return TDecompositionResultDescr(TDecompositionResultDescr::DR_W, Id2Info.At(id).Frequency, 0);
        else
            return TDecompositionResultDescr();
    }

    ui32 Size() const {
        return Id2Info.GetSize();
    }

    TAutoPtr<TLexicalDecomposition> CreateDecompositor(const TUtf16String& token, ui32 options, const TDecompositionResultDescr& descr) {
        TAhoSearchResult<ui32> ahoSearch = AhoSearcher.AhoSearch(token);

        TLexicalDecomposition::TEndposPtr endpos(new TVector<ui32>(ahoSearch.size()));
        TLexicalDecomposition::TAdditionalInfoPtr wordInfo(new TWordAdditionalInfoArr());
        wordInfo->reserve(ahoSearch.size());
        for (size_t i = 0; i < ahoSearch.size(); ++i) {
            (*endpos)[i] = ahoSearch[i].first + 1;
            wordInfo->push_back(&Id2Info.At(ahoSearch[i].second));
        }

        return TAutoPtr<TLexicalDecomposition>(new TLexicalDecomposition(options, token.length(), endpos, wordInfo, descr));
    }

private:
    typedef TMappedAhoCorasick<TUtf16String, ui32, TMappedSingleOutputContainer<ui32>> TAhoSearcher;

    TCompactTrie<wchar16, ui32> Word2Id;
    TYVector<TWordAdditionalInfo> Id2Info;
    TAhoSearcher AhoSearcher;
};

TTokenLexicalSplitter::TTokenLexicalSplitter(const TBlob& blob, bool, bool visualize, ui32 options)
    : Blob(blob)
    , Reader(Blob)
    , Visualize(visualize)
    , Options(options)
    , ResultLanguage(LANG_UNK)
    , BestProcessedTokenId(0)
    , ProcessedTokensAmount(0)
{
    const ui32 version = TSingleValue<ui32>(Reader.GetBlobByName("Version")).Get();
    if (version != VOCABULARY_VERSION)
        ythrow yexception() << "Unknown version: " << version << " instead of " << TAhoCorasickCommon::GetVersion();
}

bool TTokenLexicalSplitter::ProcessToken(const TUtf16String& token, ELanguage language) {
    ++ProcessedTokensAmount;

    TBlob vocabularyData;
    if (!Reader.GetBlobByName(NameByLanguage(language), vocabularyData))
        return false;

    try {
        static THolder<TFixedBufferFileOutput> visual(Visualize ? new TFixedBufferFileOutput("visualize.txt") : nullptr);

        TSpecifiedVocabulary words(vocabularyData);

        /// if token is in the vocabulary
        if (ResultDescr.Relax(words.CreateSingleWordDescr(token))) {
            ResultWords = TVector<TUtf16String>(1, token);
            ResultLanguage = language;
            BestProcessedTokenId = ProcessedTokensAmount - 1;
            return true; /// better than a word can be only an another word
        }

        TAutoPtr<TLexicalDecomposition> decompositor = words.CreateDecompositor(token, Options, ResultDescr);
        if (Options & DO_MANUAL) {
            decompositor->DoDecompositionManual();
        }
        if (Options & DO_GENERAL) {
            decompositor->DoDecomposition();
        }
        if (!decompositor->WasImproved() || decompositor->ResultIsUgly())
            return false;

        ResultDescr = decompositor->GetDescr();
        decompositor->SaveResult(token, ResultWords);
        ResultLanguage = language;
        BestProcessedTokenId = ProcessedTokensAmount - 1;

        if (Visualize) {
            *visual << WideToUTF8(token) << ", language: " << NameByLanguage(language) << Endl;
            decompositor->DoVisualize(token, *visual);
        }

        return true;
    } catch (const yexception& e) {
        ythrow yexception() << "Cannot process token " << token << ": " << e.what() << Endl;
    }
}

const TUtf16String& TTokenLexicalSplitter::operator[](size_t index) const {
    if (index >= ResultWords.size())
        ythrow yexception() << "bad index: " << index << " of " << ResultWords.size();
    return ResultWords[index];
}

const TString TTokenLexicalSplitter::ProcessUntranslitOutput(const TString& line) {
    const static TSplitDelimiters DELIMS(" ");
    const TDelimitersSplit split(line, DELIMS);
    TDelimitersSplit::TIterator it = split.Iterator();

    const TString token = it.NextString();
    ProcessToken(CharToWide(token, CODES_WIN), LANG_ENG);
    while (!it.Eof()) {
        ProcessToken(CharToWide(it.NextString(), CODES_WIN), LANG_RUS);
    }
    return token;
}
