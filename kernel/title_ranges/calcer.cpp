#include "calcer.h"

#include <ysite/yandex/reqanalysis/normalize.h>

#include <library/cpp/resource/resource.h>

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/text/word_input_symbol.h>
#include <kernel/remorph/text/word_symbol_factory.h>
#include <library/cpp/solve_ambig/solve_ambiguity.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/generic/bitmap.h>
#include <util/generic/hash.h>
#include <util/generic/typetraits.h>
#include <util/generic/algorithm.h>
#include <util/ysaveload.h>
#include <library/cpp/bit_io/bitoutput.h>
#include <library/cpp/bit_io/bitinput.h>
#include <util/stream/output.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <util/string/split.h>

// List of marker words for fast filtering.
// Most of document titles don't need to be parsed with Remorph.
// Checking for marker words first results in significant speedup.
//
static const char* ClassFilterRE =
    "(сери|сері|выпуск|випуск|эпизод|эпізод|episode|serie|bölüm|сезон|season|sezon|saison|класс)";

static const char* RangeFilterRE =
    "(все|all|нов|последн|new|last|\\d{1,3})";

class TDocTitleRangesCalcer::TImpl {
private:
    enum EMatchType {
        MtSeason,
        MtEpisode,
        MtClass,

        MtNumber,
        MtAll,
        MtNew,

        MtNumItems
    };

    typedef TBitMap<MtNumItems> TMatchTypeSet;
    typedef THashMap<TString, EMatchType> TTagMap;

public:
    TImpl(TQueryRecognizerPtr queryRecognizer)
    : QueryRecognizer(queryRecognizer)
    , ClassRE(ClassFilterRE)
    , RangeRE(RangeFilterRE)
    {
        InitTagMap();
        InitMatcher();
    }

    bool CalcRanges(const TUtf16String& title, TDocTitleRanges& ranges, const TLangMask& mask) const
    {
        // Cerr << "-D- CalcRanges for \"" << title << "\"" << Endl;

        TUtf16String wTitle = PreprocessText(title.size() <= 1024 ? title : title.substr(0, 1024));

        TString titleUtf8 = WideToUTF8(wTitle);
        if (!ClassRE.Match(titleUtf8.data()) || !RangeRE.Match(titleUtf8.data())) {
            return false;
        }

        Fill(ranges.begin(), ranges.end(), TDocTitleRange());

        TLangMask langMask;

        if (!mask.Empty()) {
            langMask = mask;
        }
        else if (QueryRecognizer.Get()) {
            langMask = QueryRecognizer->RecognizeParsedQueryLanguage(wTitle).GetLangMask();
        }
        else {
            langMask = TLangMask(LANG_RUS, LANG_ENG, LANG_TUR, LANG_UKR);
        }

        // Cerr << "-D- Looking for matches in \"" << wTitle << "\"" << Endl;

        NText::TWordSymbols symbols = NText::CreateWordSymbols(wTitle, langMask, NToken::MS_ALL);

        NReMorph::TMatchResults res;
        Matcher->SearchAll(symbols, res);
        NSolveAmbig::SolveAmbiguity(res);

        bool nonTrivial = false;

        for (NReMorph::TMatchResults::const_iterator it = res.begin(), itEnd = res.end(); it != itEnd; ++it) {
            NReMorph::TMatchResult& match = **it;
            TStringBuf ruleName(match.RuleName.data(), match.RuleName.size());

            TDocTitleRange range;
            EDocTitleRangeType rangeType = ProcessMatchResult(symbols, **it, range);

            if (!range.Empty()) {
                nonTrivial = true;
            }

            ranges[rangeType].Update(range);
        }

        return nonTrivial;
    }

private:
    void InitTagMap()
    {
        TagMap = {
            {"number_left",  MtNumber},
            {"number_right", MtNumber},
            {"all_left",     MtAll},
            {"all_right",    MtAll},
            {"new_left",     MtNew},
            {"new_right",    MtNew},
            {"season",       MtSeason},
            {"episode",      MtEpisode},
            {"class",        MtClass}
        };
    }

    void InitMatcher()
    {
        TString rulesStr = NResource::Find("title_ranges_remorph_rules");
        Matcher = NReMorph::TMatcher::Parse(rulesStr, nullptr);
    }

    TUtf16String PreprocessText(const TUtf16String& text) const {
        TUtf16String res;
        res.reserve(text.size());
        bool space = false;

        for (size_t i = 0; i != text.size(); ++i) {
            if (!IsAlnum(text[i])) {
                space = true;
            }
            else {
                if (space && !res.empty()) {
                    res.append(' ');
                }
                res.append(ToLower(text[i]));
                space = false;
            }
        }

        return res;
    }

    void CheckMatchTypes(const TMatchTypeSet& matchTypes) const
    {
        TMatchTypeSet keywordTypes;
        keywordTypes.Set(MtSeason).Set(MtEpisode).Set(MtClass);

        TMatchTypeSet dataTypes;
        dataTypes.Set(MtNumber).Set(MtAll).Set(MtNew);

        size_t keywordCount = keywordTypes.And(matchTypes).Count();
        size_t dataCount = dataTypes.And(matchTypes).Count();

        if (keywordCount != 1) {
            ythrow yexception() << "Error parsing numeric range from title (" << keywordCount << " keywords in one rule). Something is not right with ReMorph rules?" << Endl;
        }

        if (dataCount != 1) {
            ythrow yexception() << "Error parsing numeric range from title (" << dataCount << " numbers in one rule). Something is not right with ReMorph rules?" << Endl;
        }
    }

    EDocTitleRangeType ProcessMatchResult(const NText::TWordSymbols& symbols,
                                         const NReMorph::TMatchResult& result,
                                         TDocTitleRange& range) const
    {
        // Cerr << "-D- Processing match result (" << result.WholeExpr.first << "," << result.WholeExpr.second << ") for rule " << result.RuleName << Endl;

        NRemorph::TNamedSubmatches submatches;
        result.GetNamedRanges(submatches);

        TMatchTypeSet matchTypes;

        for (NRemorph::TNamedSubmatches::const_iterator it = submatches.begin(); it != submatches.end(); ++it) {
            EMatchType matchType = ProcessSubmatch(symbols, result, it->first, it->second, range);

            matchTypes.Set(matchType);
        }

        CheckMatchTypes(matchTypes);

        if (matchTypes.Test(MtSeason)) {
            return DRT_SEASON;
        }
        else if (matchTypes.Test(MtEpisode)) {
            return DRT_EPISODE;
        }
        else if (matchTypes.Test(MtClass)) {
            return DRT_CLASS;
        }

        Y_FAIL("Unknown match type.");
    }

    EMatchType ProcessSubmatch(const NText::TWordSymbols& symbols,
                               const NReMorph::TMatchResult& result,
                               const TString& matchTag,
                               const NRemorph::TSubmatch& submatch,
                               TDocTitleRange& range) const
    {
        // Cerr << "-D- Processing submatch (" << submatch.first << "-" << submatch.second << ") with tag " << matchTag << Endl;

        TTagMap::const_iterator it = TagMap.find(matchTag);

        if (it != TagMap.end()) {
            EMatchType matchType = it->second;

            TDocTitleRange foundRange;
            if (matchType == MtNumber) {
                for (size_t i = submatch.first; i != submatch.second; ++i) {
                    NText::TWordInputSymbolPtr symbol = symbols[result.WholeExpr.first + i];

                    ui32 number;
                    if (TryFromString<ui32>(ToString(symbol), number)) {
                        foundRange.Add(number);
                    }
                }

                if (foundRange.Empty()) {
                    ythrow yexception() << "Error parsing numeric range from title (no numbers found). Something is not right with ReMorph rules?" << Endl;
                }
            }
            else if (matchType == MtAll) {
                foundRange.SetAll();
            }
            else if (matchType == MtNew) {
                // Do nothing, not implemented yet
            }

            if (!foundRange.Empty()) {
                range.Update(foundRange);
            }

            return matchType;
        }

        ythrow yexception() << "Error parsing numeric range from title (invalid tag). Something is not right with ReMorph rules?" << Endl;
    }

private:
    TTagMap                       TagMap;
    TQueryRecognizerPtr           QueryRecognizer;
    TMatcherPtr                   Matcher;
    TRegExMatch                   ClassRE;
    TRegExMatch                   RangeRE;
};

/////////////////////////////////////////

TDocTitleRangesCalcer::TDocTitleRangesCalcer(TQueryRecognizerPtr queryRecognizer)
: Impl(new TImpl(queryRecognizer))
{
}

TDocTitleRangesCalcer::~TDocTitleRangesCalcer()
{
}

bool TDocTitleRangesCalcer::CalcRanges(const TUtf16String& title, TDocTitleRanges& ranges, const TLangMask& mask) const
{
    return Impl->CalcRanges(title, ranges, mask);
}
