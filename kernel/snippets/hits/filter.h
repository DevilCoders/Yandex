#pragma once

#include "topn.h"
#include "sbitset.h"

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets
{
    // ui16 is sufficient as uforms contain word forms that are less than NFORM_LEVEL_Max
    static_assert(NFORM_LEVEL_Max <= sizeof(ui16)*8, "expect NFORM_LEVEL_Max <= sizeof(ui16)*8");
    typedef TShortBitSet<ui16> TBitSet;

    class ISentChecker : private TNonCopyable
    {
    public:
        virtual bool IsBad(ui16 sent, const TBitSet& uforms) = 0;
        virtual ~ISentChecker() {}
    };

    struct THitsCountSentCmp {
        //first - hits count
        //second - sent
        bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) {
            if (a.first != b.first) {
                return a.first > b.first;
            }
            return a.second < b.second;
        }
    };

    class TMultiTopFilter
    {
    private:
        THolder<ISentChecker> Checker;
        bool NeedAll;
        TVector<ui16>& Sents;
        TMultiTop<std::pair<int, int>, THitsCountSentCmp, float> Top;
    public:
        TMultiTopFilter(TVector<ui16>& sents, const TVector<float>& weights, int maxCnt, bool needAllPassageHits)
            : NeedAll(needAllPassageHits)
            , Sents(sents)
            , Top(maxCnt, weights, needAllPassageHits)
        {
            Sents.clear();
        }

        void SetChecker(ISentChecker* checker)
        {
            Checker.Reset(checker);
        }

        void ProcessSent(const ui16& sent, TBitSet& forms)
        {
            if (NeedAll) {
                Sents.push_back(sent);
                return;
            }
            if (Checker.Get() && Checker->IsBad(sent, forms))
                return;
            const size_t c = forms.Size(); // remember how many query words in sent
            for (const size_t& fit = forms.Begin(); !forms.End(); forms.Next()) {
                Top.Push(fit, std::make_pair(c, sent));
            }
        }

        void Finilize()
        {
            if (!NeedAll) {
                TVector<std::pair<int, int>> tops = Top.ToVector();
                Sents.reserve(tops.size());
                for (size_t i = 0; i < tops.size(); ++i) {
                    Sents.push_back(tops[i].second);
                }
                Sort(Sents.begin(), Sents.end());
            }
        }
    };

}

