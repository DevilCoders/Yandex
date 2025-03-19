#pragma once

#include <kernel/snippets/iface/archive/sent.h>

#include <util/generic/algorithm.h>
#include <util/generic/list.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/charset/wide.h>
#include <utility>

namespace NSnippets {
    class TArchiveView;
    class TArchiveViews;
    class TSentsRange : public TIntrusiveListItem<TSentsRange> {
    public:
        int FirstId = 0;
        int LastId = 0;
        TArchiveSentList::TConstIterator ResultBeg;
        TArchiveSentList::TConstIterator ResultEnd;
        TSentsRange() {
        }
        TSentsRange(int firstId, int lastId)
            : FirstId(firstId)
            , LastId(lastId)
        {
        }
    };
    typedef TIntrusiveListWithAutoDelete<TSentsRange, TDelete> TSentsRangeList;
    class TSentsOrder {
    public:
        TSentsRangeList Sents;
        void PushBack(int first, int last) {
            if (first <= last && last > 0) {
                if (first < 1) {
                    first = 1;
                }
                Sents.PushBack(new TSentsRange(first, last));
            }
        }
        void Clear() {
            Sents.Clear();
        }
        bool Empty() const {
            return Sents.Empty();
        }
    };
    void DumpResultCopy(const TSentsOrder& all, TVector<TUtf16String>& res);
    void DumpResultCopy(const TSentsOrder& all, TVector< TVector<TUtf16String> >& res);
    void DumpIds(const TSentsOrder& all, TVector<int>& res);
    void DumpAttrs(const TSentsOrder& all, TVector<TString>& res);
    void DumpResult(const TSentsOrder& all, TArchiveView& res, bool allowEmptySent = false);
    void DumpResult(const TSentsOrder& all, TArchiveViews& res, bool allowEmptySent = false);
    class TSentsOrderGenerator {
    public:
        TSentsOrder Result;
        TVector<std::pair<int, int>> Temp;
        void PushBack(int first, int last) {
            Temp.push_back(std::make_pair(first, last));
        }
        void Sort() {
            ::Sort(Temp.begin(), Temp.end());
        }
        void SortAndMerge() {
            if (!Temp.size()) {
                return;
            }
            Sort();
            size_t dst = 1;
            for (size_t i = 1; i != Temp.size(); ++i) {
                if (Temp[i].first > Temp[dst - 1].second) {
                    Temp[dst++] = Temp[i];
                } else {
                    if (Temp[i].second > Temp[dst - 1].second) {
                        Temp[dst - 1].second = Temp[i].second;
                    }
                }
            }
            Temp.resize(dst);
        }
        static void Cutoff(const TSentsOrder& ex, TSentsOrder& result);
        void Complete(TSentsOrder* res = nullptr) {
            if (!res) {
                res = &Result;
            }
            for (size_t i = 0; i != Temp.size(); ++i) {
                res->PushBack(Temp[i].first, Temp[i].second);
            }
            Temp.clear();
        }
    };
}
