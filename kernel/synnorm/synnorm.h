#pragma once

#include <kernel/qtree/richrequest/richnode.h>
#include <util/generic/string.h>
#include <library/cpp/testing/unittest/registar.h>

// Forward declarations.
class TGztResults;
struct IBinSaver;

namespace NSynNorm {

// Implements "synnorm" normalization, which means replacing
// words (or even collocations) with theirs synsets. Also
// it includes excluding stopwords and sorting resulting synsets.
// Can be used autonomously (builds rich tree itself, has it's own gazetteer),
// and can be used in context of wizard (uses wizard's rich tree, wizard's gazetteer).
class TSynNormalizer {
private:
    class TImpl;

public:
    TSynNormalizer();
    ~TSynNormalizer();

    int operator& (IBinSaver& saver);

    // You should do it if you want to use autonomously
    // (otherwise no synsets will be found).
    void LoadSynsets(const TString& gztBinPath);
    void LoadSynsets(const TBlob& gztData);

    // You should do it if you want to use autonomously
    // (if you omit loading stopwords, they will not be removed,
    // except some specific hardcoded in synnorm.cpp,
    // stolen from doppelgangers normalization for better compatiblity).
    // Given stopwords will be used in CreateRichTree.
    // The file is typically called stopword.lst
    // (for example arcadia_tests_data/wizard/language/stopwords.lst)
    void LoadStopwords(const TString& stopwordsPath);
    void LoadStopwords(const TBlob& stopwordsData);

    // Build rich tree, makes search with internal gazetteer, calls main method.
    TUtf16String Normalize(const TUtf16String& query,
                     const TLangMask& langMask = TLangMask(LANG_ENG, LANG_RUS),
                     bool sortWords = true) const;

    // The same with utf-8 decoding and encoding.
    TString NormalizeUTF8(const TString& query,
                         const TLangMask& langMask = TLangMask(LANG_ENG, LANG_RUS),
                         bool sortWords = true) const;

    // <root> is root of rich tree built from query.
    // <gztResults> contains all results of gazetteer <root> processing.
    // There can be articles other than TMember's in <gztResults>, they'll be skipped.
    TUtf16String Normalize(TRichNodePtr root,
                     const TGztResults& gztResults,
                     bool sortWords = true) const;

    // Returns human readable summary of query analysis for debug purposes.
    TString DebugRequestSummaryString(const TString& query,
                                     const TLangMask& langMask = TLangMask(LANG_ENG, LANG_RUS),
                                     bool sortWords = true) const;

private:
    THolder<TImpl> Impl_;
};

TString NormalizeWildcardsUsingSynnorm(const TStringBuf& synnormRequest);
TUtf16String NormalizeWildcardsUsingSynnorm(const TUtf16String& synnormRequest);

};
