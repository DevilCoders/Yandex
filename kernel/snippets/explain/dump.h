#pragma once

#include <kernel/snippets/sent_match/callback.h>

#include <kernel/snippets/factors/factor_storage.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/algo/redump.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NSnippets
{
    class TConfig;

   struct TSnipStat {
        double SnipWeight = 0.0;
        TFactorStorage Factors;
        TVector<TString> TextForHtml;
        using TWordRanges = TVector<std::pair<int, int>>;
        TWordRanges WordRanges;
        TString ArcCoords;
        const TUtf16String Title;

        TSnipStat(double snipWeight, const TFactorStorage& factors, const TVector<TString>& textForHtml, const TWordRanges& wordRanges, const TString& arcCoords, const TUtf16String& title)
          : SnipWeight(snipWeight)
          , Factors(factors)
          , TextForHtml(textForHtml)
          , WordRanges(wordRanges)
          , ArcCoords(arcCoords)
          , Title(title)
        {
        }
    };

    struct TAlgoTop : IAlgoTop {
        typedef TSimpleSharedPtr<TSnipStat> TValuePtr;
        typedef TVector<TValuePtr> TStorage;
        TString Name;
        TStorage Top;

        TAlgoTop(const TString& name)
          : Name(name)
        {
        }

        void Push(const TSnip& snip, const TUtf16String& title = TUtf16String()) override;

        static bool Cmp(const TValuePtr& a, const TValuePtr& b) {
            return a->SnipWeight > b->SnipWeight;
        }

        const TStorage GetSortedSnips() const {
            TStorage res = Top;
            StableSort(res.begin(), res.end(), Cmp);
            return res;
        }
    };

    typedef TList<TAlgoTop> TTops;
    class TDumpCallback : public ISnippetsCallback
    {
    private:
      class TImpl;
      THolder<TImpl> Impl;

    public:
      TDumpCallback(const TConfig& cfg);
      ~TDumpCallback() override;

      ISnippetCandidateDebugHandler* GetCandidateHandler() override;

      void GetExplanation(IOutputStream& output) const override;
    };

}
