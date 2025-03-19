#pragma once
#include "tags.h"
#include "snippet_reader.h"
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/system/tls.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
struct ISnippetClassifier {
    virtual int GetTags(const TStringBuf& snip) const = 0;
    virtual ~ISnippetClassifier() {
    }
};
ISnippetClassifier* CreateSnipClassifier();
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef THashMap<TUtf16String, TVector<TUtf16String> > TCategories;
////////////////////////////////////////////////////////////////////////////////////////////////////
class TFragmentsCollection {
public:
    struct TTagHandle {
        int Idx;
        ui32 Mask;
        int Variant;
    };
    struct TTagFragment {
        TString Tag;
        ui32 FullMask;
        TVector<TUtf16String> Variants;
        TVector<size_t> LenUTF;
    };
private:
    TVector<TTagFragment> TagFragments;
    TSearcherAutomaton<TTagHandle> Searcher;
    mutable Y_THREAD(TVector<ui32>) CurMasks;
    mutable Y_THREAD(TVector<TVector<int> >) WordsFound;
private:
    void AddCateg(const TUtf16String &categName, const TVector<TUtf16String> &variants, int idx, ui32 mask, TTagFragment* tq);
    void AddFragments(const TUtf16String& templ, int idx, ui32 mask, TTagFragment* tq);
public:
    void UseTurkishLanguage(bool yes);
    void AddTagFragments(const TString& tag, const TVector<TUtf16String>& fields);
    void AddTagFragments(const TString& tag, const TVector<TUtf16String>& fields, const TCategories &categories);
    void Finalize();
    void RecognizeTags(const TStringBuf& request, TVector<TString>* tags) const;
    struct TTagExplained {
        TString Tag;
        TVector<TUtf16String> WordsWhy;
    };
    void RecognizeTagsExplained(const TStringBuf& request, TVector<TTagExplained>* tags) const;
    struct TMasks {
        ui64 Words;
        ui32 Fragments;
    };
    void RecognizeTagsWithPositions(const TStringBuf& request, const TVector<size_t>& wordsStarts, TVector<TMasks>* tags) const;
    //
    bool HasTag(const TString& tag) const;
    bool IsKnownFragment(const TString& tag, const TUtf16String& fragment) const;
};
