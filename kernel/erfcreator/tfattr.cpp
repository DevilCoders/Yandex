#include <util/stream/mem.h>
#include <util/string/cast.h>
#include <library/cpp/deprecated/split/split_iterator.h>

#include <library/cpp/deprecated/dater_old/structs.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <ysite/yandex/erf_format/erf_format.h>

#include "tfattr.h"

double FromStringImplNoThrow(const char* data, size_t len) {
    char* se;
    return StrToD(data, data + len, &se);
}

double FromStringImplNoThrow(const TString& data) {
    return FromStringImplNoThrow(data.data(), data.size());
}

#define ATTR_TO_ERF_NO_THROW(ARCHNAME, ERFNAME) { \
                                            TErfAttrs::const_iterator it = erfAttrs.find(ARCHNAME); \
                                            if (it != erfAttrs.end() && ! it->second.empty()) \
                                                erf.ERFNAME = ClampVal<ui32>((ui32)(FromStringImplNoThrow(it->second.data(), it->second.size())*255.0), 0, 255); \
                                       }

#define ATTR_TO_ERF(ARCHNAME, ERFNAME) { \
                                            TErfAttrs::const_iterator it = erfAttrs.find(ARCHNAME); \
                                            if (it != erfAttrs.end() && ! it->second.empty()) \
                                                erf.ERFNAME = ClampVal<ui32>((ui32)(FromString<float>(it->second)*255.0), 0, 255); \
                                       }

#define BOOL_ATTR_TO_ERF(ARCHNAME, ERFNAME) { \
                                            TErfAttrs::const_iterator it = erfAttrs.find(ARCHNAME); \
                                            if (it != erfAttrs.end() && ! it->second.empty()) \
                                                erf.ERFNAME = ClampVal<ui32>((ui32)(FromString<ui32>(it->second)), 0, 1); \
                                       }

static void ParseDate(const TString& str, int& day, int& month, int& year)
{
    const static TSplitDelimiters DELIMS(".");
    const TDelimitersSplit split(str, DELIMS);
    TDelimitersSplit::TIterator it = split.Iterator();
    day = ::FromString<int>(it.NextString());
    month = ::FromString<int>(it.NextString());
    year = ::FromString<int>(it.NextString());
    if (!it.Eof())
        ythrow yexception() << "bad date string";
}

void FillErfTextFeatures(const TErfAttrs& erfAttrs, SDocErfInfo3& erf) {
    ATTR_TO_ERF("Language2", Language2);
    ATTR_TO_ERF("TextF", TextFeatures);
    ATTR_TO_ERF("TextL", TextLike);
    ATTR_TO_ERF("Syn7bV", SynS1);
    ATTR_TO_ERF_NO_THROW("Syn8bV", SynFLremap1);
    ATTR_TO_ERF("Syn9aV", SynFLremap2);
    ATTR_TO_ERF("SynPercentBadWordPairs", SynPercentBadWordPairs);
    ATTR_TO_ERF_NO_THROW("SynNumBadWordPairs", SynNumBadWordPairs);
    ATTR_TO_ERF("NumLatinLetters", NumLatinLetters);
    ATTR_TO_ERF("RusWordsInText", WordsInText);
    ATTR_TO_ERF("RusWordsInTitle", WordsInTitle);
    ATTR_TO_ERF("MeanWordLength", MeanWordLength);
    ATTR_TO_ERF("PercentWordsInLinks", PercentWordsInLinks);
    ATTR_TO_ERF("PercentVisibleContent", PercentVisibleContent);
    ATTR_TO_ERF("PercentFreqWords", PercentFreqWords);
    ATTR_TO_ERF("PercentUsedFreqWords", PercentUsedFreqWords);
    ATTR_TO_ERF("TrigramsProb", TrigramsProb);
    ATTR_TO_ERF("TrigramsCondProb", TrigramsCondProb);
    BOOL_ATTR_TO_ERF("Eshop", IsCommEshop);

    TErfAttrs::const_iterator toEshopV = erfAttrs.find("EshopV");
    if (toEshopV != erfAttrs.end() && ! toEshopV->second.empty())
        erf.Eshop = ClampVal<ui32>((ui32)(FromString<float>(toEshopV->second) * 16.0), 0, 15);

    TErfAttrs::const_iterator toPornoV = erfAttrs.find("PornoV");
    if (toPornoV != erfAttrs.end() && ! toPornoV->second.empty())
        erf.PornoValue = ClampVal<ui32>((ui32)(FromString<float>(toPornoV->second) * 16.0), 0, 15);

    TErfAttrs::const_iterator toRecDate = erfAttrs.find("RecD");
    int day, month, year;
    if (toRecDate != erfAttrs.end() && ! toRecDate->second.empty()) {
        ParseDate(toRecDate->second, day, month, year);
        erf.DocDateDay = day;
        erf.DocDateMonth = month;
        erf.DocDateYear = year;
    }

    ATTR_TO_ERF("Poetry", Poetry);
    ATTR_TO_ERF("Poetry2", PoetryQuad);

    TErfAttrs::const_iterator toBreaks = erfAttrs.find("Breaks");
    if (toBreaks != erfAttrs.end() && ! toBreaks->second.empty()) {
        float value = (float)FromStringImplNoThrow(toBreaks->second)/2560;
        erf.DocLen = (ui8)ClampVal((int)(value*255.0), 0, 255);
    }

    TErfAttrs::const_iterator toDocSize = erfAttrs.find("DocSize");
    if (toDocSize != erfAttrs.end() && ! toDocSize->second.empty()) {
        float value = log(1 + (float)FromStringImplNoThrow(toDocSize->second)) / 15;
        erf.DocSize = (ui8)ClampVal((int)(value*255.0), 0, 255);
    }

    TErfAttrs::const_iterator toLT = erfAttrs.find("LongText");
    if (toLT != erfAttrs.end() && ! toLT->second.empty()) {
        ui32 breaks = FromString<ui32>(toLT->second);
        erf.nLongPureText = (breaks > 50) ? 1 : 0;
        erf.LongestText = (ui8)ClampVal<ui32>(breaks, 0, 255);
    }

    ATTR_TO_ERF("NumeralsPortion", NumeralsPortion);
    ATTR_TO_ERF("ParticlesPortion", ParticlesPortion);
    ATTR_TO_ERF("AdjPronounsPortion", AdjPronounsPortion);
    ATTR_TO_ERF("AdvPronounsPortion", AdvPronounsPortion);
    ATTR_TO_ERF("VerbsPortion", VerbsPortion);
    ATTR_TO_ERF("FemAndMasNounsPortion", FemAndMasNounsPortion);
    ATTR_TO_ERF("SegmentAuxAlphasInText", SegmentAuxAlphasInText);
    ATTR_TO_ERF("SegmentAuxSpacesInText", SegmentAuxSpacesInText);
    ATTR_TO_ERF("SegmentContentCommasInText", SegmentContentCommasInText);
    ATTR_TO_ERF("SegmentWordPortionFromMainContent", SegmentWordPortionFromMainContent);
    ATTR_TO_ERF("Soft404", Soft404);
    BOOL_ATTR_TO_ERF("IsShop", IsShop);
    BOOL_ATTR_TO_ERF("IsReview", IsReview);
    BOOL_ATTR_TO_ERF("IsEbookForRead", IsEbookForRead);
    BOOL_ATTR_TO_ERF("SmartSoft404", SmartSoft404);
    BOOL_ATTR_TO_ERF("HasUserReviewsH", HasUserReviewH);

    TErfAttrs::const_iterator toFirstPostDate = erfAttrs.find("FirstPostDate");
    if (toFirstPostDate != erfAttrs.end() && ! toFirstPostDate->second.empty()) {
        ParseDate(toFirstPostDate->second, day, month, year);
        erf.FirstPostDateDay = day;
        erf.FirstPostDateMonth = month;
        erf.FirstPostDateYear = year;
    }
    TErfAttrs::const_iterator toLastPostDate = erfAttrs.find("LastPostDate");
    if (toLastPostDate != erfAttrs.end() && ! toLastPostDate->second.empty()) {
        ParseDate(toLastPostDate->second, day, month, year);
        erf.LastPostDateDay = day;
        erf.LastPostDateMonth = month;
        erf.LastPostDateYear = year;
    }
    TErfAttrs::const_iterator toNumForumPosts = erfAttrs.find("NumForumPosts");
    if (toNumForumPosts != erfAttrs.end() && ! toNumForumPosts->second.empty())
        erf.NumForumPosts = (ui8)ClampVal<ui32>(FromStringImplNoThrow(toNumForumPosts->second), 0, 255);
    TErfAttrs::const_iterator toNumForumAuthors = erfAttrs.find("NumForumAuthors");
    if (toNumForumAuthors != erfAttrs.end() && ! toNumForumAuthors->second.empty())
        erf.NumForumAuthors = (ui8)ClampVal<ui32>(FromStringImplNoThrow(toNumForumAuthors->second), 0, 255);
}

