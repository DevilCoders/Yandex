#pragma once

#include <kernel/snippets/config/enums.h>
#include <kernel/snippets/sent_match/retained_info.h>

#include <kernel/snippets/idl/enums.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSnippets
{
    class TConfig;
    class TSentsInfo;
    class TSentsMatchInfo;
    class TQueryy;
    class TSnip;
    class TEQInfo;

    struct TTitleFactors {
    public:
        float PixelLength = 0.0f;
        double PLMScore = -Max<double>();
        double LogMatchIdfSum = 0.0;
        double MatchUserIdfSum = 0.0;
        int SynonymsCount = 0;
        double QueryWordsRatio = 0.0;
        bool HasRegionMatch = false;
        bool HasBreak = false;
    };

    class TDefinition {
    private:
        TUtf16String DefinitionString = TUtf16String();
        EDefSearchResultType DefinitionType = NOT_FOUND;
    public:
        TDefinition()
        {
        }
        TDefinition(const TUtf16String& definitionString, EDefSearchResultType definitionType)
           : DefinitionString(definitionString)
           , DefinitionType(definitionType)
        {
        }
        const TUtf16String& GetDefinitionString() const {
            return DefinitionString;
        }
        EDefSearchResultType GetDefinitionType() const {
            return DefinitionType;
        }
    };

    class TSnipTitle {
    private:
        struct TImpl;
        THolder<TImpl> Impl;

    public:
        const TSentsInfo* GetSentsInfo() const;
        const TSentsMatchInfo* GetSentsMatchInfo() const;
        const TUtf16String& GetTitleString() const;
        float GetPixelLength() const;
        const TSnip& GetTitleSnip() const;
        double GetPLMScore() const;
        double GetLogMatchIdfSum() const;
        double GetMatchUserIdfSum() const;
        int GetSynonymsCount() const;
        double GetQueryWordsRatio() const;
        bool HasRegionMatch() const;
        bool HasBreak() const;
        const TEQInfo& GetEQInfo() const;
        const TDefinition& GetDefinition() const;

    public:
        TSnipTitle();
        TSnipTitle(const TUtf16String& titleString, const TConfig& cfg, const TQueryy& query);
        TSnipTitle(const TUtf16String& titleString, const TRetainedSentsMatchInfo& source, const TSnip& titleSnip, const TTitleFactors& titleFactors, const TDefinition& definition);
        ~TSnipTitle();
        TSnipTitle(const TSnipTitle& other);
        TSnipTitle& operator=(const TSnipTitle& other);
    };

    bool SimilarTitleStrings(const TUtf16String& string1, const TUtf16String& string2, double ratio, bool ignoreDiacriticsForTurkey = false);

    TString GetLatinLettersAndDigits(const TWtringBuf& str);

    void EraseFullStop(TUtf16String& line);

    const TUtf16String& GetTitleSeparator(const TConfig& cfg);

    bool IsCandidateBetterThanTitle(const TQueryy& query, const TSnipTitle& candidateInfo, const TSnipTitle& titleInfo, bool altheaders3, bool glued);
}
