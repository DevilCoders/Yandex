#include "snippet_classifier.h"
#include "snippet_reader.h"
#include <kernel/yawklib/phrase_iterator.h>
#include <ysite/yandex/doppelgangers/normalize.h>
#include <util/generic/algorithm.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
class TSnippetClassifier: public ISnippetClassifier {
    typedef TSearcherAutomaton<EResultTag> TReaderImpl;
    TReaderImpl Reader;

    struct TAllMatches {
        int Tags;
        bool Do(EResultTag tag, size_t) {
            Tags |= tag;
            return false;
        }
        TAllMatches()
            : Tags(0) {
        }
    };

    void AddTemplate(const TString& templ, EResultTag tag) {
        THolder<IPhraseIterator> iter(CreatePhraseIterator(UTF8ToWide(templ)));
        do {
            Reader.AddWtroka(iter->Get(), tag);
        } while (iter->Next());
    }
public:
    TSnippetClassifier() {
        for (TTagSnipTestData* p = TagSnipTests; p->Tag; ++p) {
            AddTemplate(p->SnipFragments, p->Tag);
        }
        Reader.Finalize();
    }
    int GetTags(const TBasicStringBuf<char>& snip) const override {
        TAllMatches test;
        Reader.Do(snip, &test);
        return test.Tags;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
ISnippetClassifier* CreateSnipClassifier() {
    return new TSnippetClassifier();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::UseTurkishLanguage(bool yes) {
    Searcher.UseTurkishLanguage(yes);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::AddCateg(const TUtf16String &categName, const TVector<TUtf16String> &variants, int idx, ui32 mask, TTagFragment* tq)
{
    TTagHandle qh;
    qh.Idx = idx;
    qh.Mask = mask;
    qh.Variant = -1;
    if (tq) {
        qh.Variant = tq->Variants.ysize();
        tq->Variants.push_back(categName);
    }
    for (size_t n = 0; n < variants.size(); ++n) {
        Searcher.AddWtroka(variants[n], qh);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::AddFragments(const TUtf16String& templ, int idx, ui32 mask, TTagFragment* tq)
{
    TTagHandle qh;
    qh.Idx = idx;
    qh.Mask = mask;
    qh.Variant = -1;
    THolder<IPhraseIterator> iter(CreatePhraseIterator(templ));
    do {
        TUtf16String variant = iter->Get();
        if (!variant.empty()) {
            if (tq) {
                qh.Variant = tq->Variants.ysize();
                tq->Variants.push_back(variant);
                size_t len = WideToUTF8(variant).length(); // could be made faster
                tq->LenUTF.push_back(len);
            }
            Searcher.AddWtroka(variant, qh);
        }
    } while (iter->Next());
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::AddTagFragments(const TString& tag, const TVector<TUtf16String>& fields, const TCategories &categories) {
    ui32 positiveBit = 2;
    int idx = (int)TagFragments.size();
    TagFragments.emplace_back();
    TagFragments.back().Tag = tag;
    for (size_t n = 0; n < fields.size(); ++n) {
        if (fields[n][0] == '-')
            AddFragments(fields[n].substr(1), idx, 1, nullptr);
        else {
            if (fields[n][0] == '{' && fields[n].back() == '}') {
                TCategories::const_iterator it = categories.find(fields[n]);
                if (it != categories.end()) {
                    const TVector<TUtf16String> &categ = it->second;
                    AddCateg(fields[n], categ, idx, positiveBit, &TagFragments.back());
                } else {
                    Cerr << "Warning: unknown categ " << fields[n] << " used, now assuming it's empty" << Endl;
                    TVector<TUtf16String> nothing;
                    AddCateg(fields[n], nothing, idx, positiveBit, &TagFragments.back());
                }
            } else
                AddFragments(fields[n], idx, positiveBit, &TagFragments.back());
            positiveBit <<= 1;
            if (positiveBit == (2u << 31))
                Cerr << "Warning: too much query fragments for " << tag << " combined with AND-op" << Endl;
        }
    }
    ui32 fullMask = positiveBit - 2;
    if (fullMask == 0)
        Cerr << "Warning: no positive examples for " << tag << Endl;
    TagFragments.back().FullMask = fullMask;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::AddTagFragments(const TString& tag, const TVector<TUtf16String>& fields) {
    TCategories empty;
    AddTagFragments(tag, fields, empty);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::Finalize() {
    Searcher.Finalize();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSiteClassifierWorker {
    TVector<ui32>& Masks;
    TSiteClassifierWorker(TVector<ui32>& masks)
        : Masks(masks) {
    }
    bool Do(const TFragmentsCollection::TTagHandle& qh, size_t) {
        Masks[qh.Idx] |= qh.Mask;
        return false;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::RecognizeTags(const TBasicStringBuf<char>& request, TVector<TString>* tags) const {
    CurMasks.Get().assign(TagFragments.size(), 0);
    TSiteClassifierWorker worker(CurMasks.Get());
    Searcher.Do(" ", request, " ", &worker);
    for (size_t n = 0; n < CurMasks.Get().size(); ++n) {
        const TTagFragment& tq = TagFragments[n];
        if (CurMasks.Get()[n] == tq.FullMask && std::find(tags->begin(), tags->end(), tq.Tag) == tags->end())
            tags->push_back(tq.Tag);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSiteClassifierExplainerWorker {
    TVector<ui32>& Masks;
    TVector<TVector<int> >& WordsFound;
    TSiteClassifierExplainerWorker(TVector<ui32>& masks, TVector<TVector<int> >& wordsFound)
        : Masks(masks)
        , WordsFound(wordsFound)
    {
    }
    bool Do(const TFragmentsCollection::TTagHandle& qh, size_t) {
        Masks[qh.Idx] |= qh.Mask;
        if (qh.Variant >= 0)
            WordsFound[qh.Idx].push_back(qh.Variant);
        return false;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::RecognizeTagsExplained(const TBasicStringBuf<char>& request, TVector<TTagExplained>* tags) const {
    CurMasks.Get().assign(TagFragments.size(), 0);
    WordsFound.Get().resize(TagFragments.size());
    for (size_t n = 0; n < TagFragments.size(); ++n)
        WordsFound.Get()[n].clear();

    TSiteClassifierExplainerWorker worker(CurMasks.Get(), WordsFound.Get());
    Searcher.Do(" ", request, " ", &worker);
    for (size_t n = 0; n < CurMasks.Get().size(); ++n) {
        const TTagFragment& tq = TagFragments[n];
        if (CurMasks.Get()[n] == tq.FullMask) {
            tags->emplace_back();
            TTagExplained& ex = tags->back();
            ex.Tag = tq.Tag;
            ex.WordsWhy.resize(WordsFound.Get()[n].size());
            for (size_t m = 0; m < ex.WordsWhy.size(); ++m)
                ex.WordsWhy[m] = tq.Variants[WordsFound.Get()[n][m]];
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSiteClassifierWithPositionsWorker {
    TVector<TFragmentsCollection::TMasks>& Masks;
    const TVector<size_t>& WordsStarts;
    const TVector<TFragmentsCollection::TTagFragment>& TagFragments;
    TSiteClassifierWithPositionsWorker(TVector<TFragmentsCollection::TMasks>& masks, const TVector<size_t>& wordsStarts,
        const TVector<TFragmentsCollection::TTagFragment>& tagFragments)
        : Masks(masks)
        , WordsStarts(wordsStarts)
        , TagFragments(tagFragments)
    {
    }
    bool Do(const TFragmentsCollection::TTagHandle& qh, size_t offset) {
        Masks[qh.Idx].Fragments |= qh.Mask;
        int wordIdx = (int) (LowerBound(WordsStarts.begin(), WordsStarts.end(), offset) - WordsStarts.begin() - 1);
        size_t startPos = offset - TagFragments[qh.Idx].LenUTF[qh.Variant];
        while (wordIdx >= 0 && WordsStarts[wordIdx] >= startPos) {
            ui64 bit = 1;
            Masks[qh.Idx].Words |= (bit << (WordsStarts.size() - wordIdx - 1));
            --wordIdx;
        }
        return false;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void TFragmentsCollection::RecognizeTagsWithPositions(const TBasicStringBuf<char>& request, const TVector<size_t>& wordsStarts,
    TVector<TMasks>* tagMasks) const
{
    TSiteClassifierWithPositionsWorker worker(*tagMasks, wordsStarts, TagFragments);
    Searcher.Do(" ", request, " ", &worker);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TFragmentsCollection::HasTag(const TString& tag) const {
    for (size_t n = 0; n < TagFragments.size(); ++n) {
        const TTagFragment& tq = TagFragments[n];
        if (tq.Tag == tag)
            return true;
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool TFragmentsCollection::IsKnownFragment(const TString& tag, const TUtf16String& fragment) const {
    for (size_t n = 0; n < TagFragments.size(); ++n) {
        const TTagFragment& tq = TagFragments[n];
        if (tq.Tag == tag) {
            for (size_t m = 0; m < tq.Variants.size(); ++m)
                if (tq.Variants[m].find(fragment) != TUtf16String::npos)
                    return true;
        }
    }
    TVector<TString> tags;
    RecognizeTags(WideToUTF8(fragment), &tags);
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
