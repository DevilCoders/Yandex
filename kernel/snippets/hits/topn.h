#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash_set.h>
#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/ymath.h>

namespace NSnippets
{
    enum EMultiTopFilterMethod {
        MTFM_DEFAULT = 0,
        MTFM_EXPLICIT_SHRINK = 1,
        MTFM_ADAPTIVE_SHRINK = 2,
        MTFM_EFFECTIVE_LEN = 4,
    };

    const int MTOP_MAX_MINLEN = 3; // max minlen value for MultiTop
    const int MTOP_MIN_MINLEN = 1; // min minlen value fot MultiTop

    template< class TValue, class TOrderCmp, class TDouble, class THashValue = hash<TValue>, class TEqualValue = std::equal_to<TValue> >
    class TMultiTop {
    private:
        int MaxSize;
        const TVector<TDouble> &Weights;
        bool NeedAll;
        bool Multi;

        typedef TVector<TValue> TTop;
        TVector<TTop> Tops;
    public:
        TMultiTop (int maxSize, const TVector<TDouble> &weights, bool needAll, bool multi = true)
            : MaxSize(maxSize)
            , Weights(weights)
            , NeedAll(needAll)
            , Multi(multi)
        {
            Init();
        }

        void Init() {
            if (Multi) {
                Tops.resize(Weights.size());
            } else {
                Tops.resize(1);
            }
        }

        void Reset() {
            Tops.clear();
            Init();
        }

        inline void Push(size_t id, const TValue &value) {
            if (Multi) {
                if (id >= Tops.size()) {
                    Tops.resize(id + 1);
                }
                Tops[id].push_back(value);
            } else {
                Tops[0].push_back(value);
            }
        }

        void PushVector(const TVector<bool> &v, const TValue &value) {
            for (size_t i = 0; i < v.size(); ++i) {
                if (v[i]) {
                    Push(i, value);
                }
            }
        }

        size_t Size() {
            size_t res = 0;
            for (size_t i = 0; i < Tops.size(); ++i) {
                res += Tops[i].size();
            }
            return res;
        }

        TVector<TValue> ToVector(ui8 fm = MTFM_DEFAULT) {
            typedef THashSet<TValue, THashValue, TEqualValue> TTopSet;
            TTopSet top(MaxSize + 3 * Tops.size());
            TOrderCmp cmp;

            // calculate overall IDF - needed further to calculate normalized query word IDFs
            TDouble sumIdf = Accumulate(Weights.begin(), Weights.end(), 0.0);
            int n = Tops.size();
            if (fm & MTFM_EFFECTIVE_LEN) {
                n = 0;
                for (size_t i = 0; i < Tops.size(); ++i) {
                    if (Tops[i].size() > 0) {
                        ++n;
                    }
                }
            }
            // prevent division by zero
            if (n == 0) {
                n = 1;
            }
            if (FuzzyEquals(sumIdf, 0)) {
                sumIdf = 1;
            }
            for (size_t i = 0; i < Tops.size(); ++i) {
                Sort(Tops[i].begin(), Tops[i].end(), cmp);

                // calculate word top length
                int len = 0;
                if (Tops.size() <= Weights.size()) {
                    // collect all hits if requested
                    len = (NeedAll ? Tops[i].size() : (1. / n - (1. / n - Weights[i] / sumIdf) / n) * MaxSize + 0.5);
                } else {
                    len = MaxSize / n;
                }

                if (fm == MTFM_DEFAULT) {
                    // top len must be not less than MTOP_MAX_MINLEN
                    len = Max(len, MTOP_MAX_MINLEN);
                } else if (fm & MTFM_ADAPTIVE_SHRINK) {
                    // top len must be not less than minLen
                    // minLen varies between 1 and 3, depends on average len
                    int minLen = Max(Min(MaxSize / n, MTOP_MAX_MINLEN), MTOP_MIN_MINLEN);
                    len = Max(len, minLen);
                }

                if (fm & MTFM_EXPLICIT_SHRINK) {
                    // explicitly sets top len to 1
                    len = 1;
                }

                int cnt = 0;
                for (typename TTop::const_iterator it = Tops[i].begin(); it != Tops[i].end() && cnt < len; ++it) {
                    if (top.insert(*it).second) {
                        ++cnt;
                    }
                }
            }

            TVector<TValue> res;
            res.reserve(top.size());
            for (typename TTopSet::const_iterator it = top.begin(); it != top.end(); ++it) {
                res.push_back(*it);
            }
            return res;
        }
        TVector<TValue> ToSortedVector(ui8 fm = MTFM_DEFAULT) {
            TVector<TValue> res = ToVector(fm);
            Sort(res.begin(), res.end(), TOrderCmp());
            return res;
        }
    };
}
