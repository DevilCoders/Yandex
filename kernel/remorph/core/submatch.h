#pragma once

#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <utility>

namespace NRemorph {

// each Submatch contains either submatch interval [begin, end) or [-1, -1]
struct TSubmatch: public std::pair<size_t, size_t> {
    inline TSubmatch()
        : std::pair<size_t, size_t>(-1, -1)
    {
    }
    inline TSubmatch(const std::pair<size_t, size_t>& p)
        : std::pair<size_t, size_t>(p)
    {
    }
    inline TSubmatch(size_t begin, size_t end)
        : std::pair<size_t, size_t>(begin, end)
    {
    }

    inline bool IsEmpty() const {
        return first == second;
    }
    inline size_t Size() const {
        return second - first;
    }
    inline TSubmatch& operator +=(size_t add) {
        first += add;
        second += add;
        return *this;
    }
    inline TSubmatch& operator -=(size_t sub) {
        Y_ASSERT(first >= sub);
        Y_ASSERT(second >= sub);
        first -= sub;
        second -= sub;
        return *this;
    }
};

typedef TVector<TSubmatch> TVectorSubmatches;

class TNamedSubmatches: public THashMultiMap<TString, TSubmatch> {
public:
    TNamedSubmatches()
        : THashMultiMap<TString, TSubmatch>()
    {
    }

    template<class TheKey>
    bool has(const TheKey& key) const {
        return find(key) != end();
    }

    bool has(const TString& key) const {
        return find(key) != end();
    }

};

inline bool operator ==(const TNamedSubmatches& ns1, const TNamedSubmatches& ns2) {
    if (ns1.size() != ns2.size()) {
        return false;
    }
    if (ns1.size() == 0) {
        return true;
    }
    TNamedSubmatches::const_iterator range1_begin = ns1.begin();
    TNamedSubmatches::const_iterator it1 = range1_begin;
    size_t range_size = 0;
    NSorted::TSortedVector<TSubmatch> range1_values;
    NSorted::TSortedVector<TSubmatch> range2_values;
    do {
        ++it1;
        ++range_size;
        if ((it1 != ns1.end()) && (it1->first == range1_begin->first)) {
            continue;
        }
        std::pair<TNamedSubmatches::const_iterator, TNamedSubmatches::const_iterator> range2 = ns2.equal_range(range1_begin->first);
        if (range2.first == ns2.end()) {
            return false;
        }
        if (range_size == 1) {
            if (range1_begin->second != range2.first->second) {
                return false;
            }
        } else {
            range1_values.reserve(range_size);
            range2_values.reserve(range_size);
            TNamedSubmatches::const_iterator range_it;
            for (range_it = range1_begin; range_it != it1; ++range_it) {
                range1_values.Insert(range_it->second);
            }
            for (range_it = range2.first; range_it != range2.second; ++range_it) {
                range2_values.Insert(range_it->second);
            }
            if (range1_values != range2_values) {
                return false;
            }
            range1_values.clear();
            range2_values.clear();
        }
        range1_begin = it1;
        range_size = 0;
    } while (it1 != ns1.end());
    return true;
}

} // NRemorph

Y_DECLARE_OUT_SPEC(inline, NRemorph::TSubmatch, s, sub) {
    s << "(" << sub.first << "," << sub.second << ")";
}

Y_DECLARE_OUT_SPEC(inline, NRemorph::TVectorSubmatches, s, v) {
    s << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i)
            s << ",";
        s << v[i];
    }
    s << "]";
}

Y_DECLARE_OUT_SPEC(inline, NRemorph::TNamedSubmatches, s, ns) {
    s << "{";
    NRemorph::TNamedSubmatches::const_iterator it = ns.begin();
    for (; it != ns.end(); ++it) {
        if (it != ns.begin())
            s << ",";
        s << it->first << "->" << it->second;
    }
    s << "}";
}
