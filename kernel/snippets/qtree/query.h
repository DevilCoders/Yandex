#pragma once

#include "squeezer.h"
#include "regionquery.h"

#include <library/cpp/langmask/langmask.h>

#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/bitmap.h>

class TRichRequestNode;

namespace NSnippets
{
    class TConfig;

    enum EQueryWordType {
        QWT_WORD,
        QWT_PHONE,
    };

    //Holds (single) Query info needed
    class TQueryy : private TNonCopyable
    {
    private:
        struct TQueryCtor;

        bool UseAlmostUserWords = false;
        int ExtRatio = 0;
        bool IgnoreDuplicateExtensions = false;
        bool ParseRequestText = false;

        int MUserPosCount = 0;
        int MNonstopUserPosCount = 0;
        int MNonstopPosCount = 0;
        int MWizardPosCount = 0;
        int MNonstopWizardPosCount = 0;

    public:
        const TRichRequestNode* const OrigTree = nullptr;
        TUtf16String OrigRequestText;
        TLangMask LangMask;
        TLangMask UserLangMask;
        bool CyrillicQuery = false;

        using TBag = TDynBitMap;
        using TBagWeight = std::pair<TBag, double>;

        struct TPositionData {
            TUtf16String OrigWord;
            bool IsStopWord = false;
            bool IsUserWord = false;
            bool IsConjunction = false;
            EQueryWordType WordType = QWT_WORD;
            double Idf = 0.0;
            double IdfNorm = 0.0;
            double IdfLog = 0.0;
            double UserIdfNorm = 0.0;
            double UserIdfLog = 0.0;
            double AlmostUserWordsIdfNorm = 0.0;
            double WizardIdfNorm = 0.0;
            THashSet<int> NeighborPositions;
            TVector<TBagWeight> Bags;
        };
        TVector<TPositionData> Positions;
        double SumIdfLog = 0.0;
        double SumUserIdfLog = 0.0;

        struct TLowerWordData {
            TVector<int> WordPositions;
        };
        THashMap<TUtf16String, TLowerWordData> LowerWords;

        THashMap<std::pair<int, int>, int> LemmaPairToForcedNeighborPos;
        int MaxForcedNeighborPosCount = 0;

        struct TLemmaData {
            bool LemmaIsPureStopWord = false;
            bool LemmaIsUserWord = false;
        };
        TVector<TLemmaData> Lemmas;

        struct TLowerFormData {
            TVector<int> LemmaIds;
            TVector<int> SynIds;
            TVector<int> AlmostUserWordsIds;
        };
        THashMap<TUtf16String, TLowerFormData> LowerForms;

        TVector<TVector<int>> Id2Poss;

        THolder<TQueryPosSqueezer> PosSqueezer;
        THolder<TRegionQuery> RegionQuery;

    public:
        TQueryy(const TRichRequestNode* origTree, const TConfig& cfg, const TRichRequestNode* regionPhraseTree = nullptr);
        int PosCount() const;
        int IdsCount() const;
        int NonstopPosCount() const;
        int UserPosCount() const;
        int NonstopUserPosCount() const;
        int WizardPosCount() const;
        int NonstopWizardPosCount() const;
        bool PositionsHasPhone(const TVector<int>& positions) const;
        bool IdsHasPhone(const TVector<int>& lemmaIds) const;
    };

}
