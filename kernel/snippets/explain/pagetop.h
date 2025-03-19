#pragma once

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

namespace NSnippets
{
    template<class TValue, class TCmp>
    class TPageTop {
    private:
        typedef std::pair<TValue, size_t> THValue;

        struct THCmp {
            TCmp Cmp;
            bool operator()(const THValue& x, const THValue& y) const {
                return Cmp(x.first, y.first) ||
                        (!Cmp(y.first, x.first) && x.second < y.second);
            }
        };

        const size_t MaxSize;
        const size_t Offset;
        size_t PushCount;
        TVector<THValue> C;
        THCmp HCmp;
    public:
        explicit TPageTop(size_t count, size_t offset)
          : MaxSize(count + offset)
          , Offset(offset)
          , PushCount(0)
        {
            C.reserve(MaxSize + 1);
        }

        void Push(const TValue& p) {
            C.push_back(std::make_pair(p, PushCount));
            ++PushCount;
            PushHeap(C.begin(), C.end(), HCmp);
            if (C.size() > MaxSize) {
                PopHeap(C.begin(), C.end(), HCmp);
                C.pop_back();
            }
        }

        size_t Size() const {
            return C.size();
        }

        void Clear() {
            C.clear();
            PushCount = 0;
        }

        TVector<TValue> ToSortedVector() const {
            TVector<TValue> res;
            if (C.size() > Offset) {
                TVector<THValue> sorted(C);
                Sort(sorted.begin(), sorted.end(), HCmp);
                res.reserve(sorted.size() - Offset);
                for (size_t i = Offset; i < sorted.size(); ++i) {
                    res.push_back(sorted[i].first);
                }
            }
            return res;
        }

        size_t GetPushCount() const {
            return PushCount;
        }
    };
}
