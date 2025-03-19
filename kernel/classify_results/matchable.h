#pragma once

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include "regex_wrapper.h"

typedef std::pair<TRxMatcher, int> TRxAndId;
typedef TVector<TRxAndId> TRxAndIds;
typedef THashMap<TUtf16String, int> TWtrokaAndIds;

////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T> bool Equals(const THashSet<T> &s1, const THashSet<T> &s2) {
    if (s1.size() != s2.size())
        return false;
    for (typename THashSet<T>::const_iterator it = s1.begin(); it != s1.end(); ++it)
        if (!s2.contains(*it))
            return false;
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class K, class V> bool Equals(const THashMap<K, V> &s1, const THashMap<K, V> &s2) {
    if (s1.size() != s2.size())
        return false;
    for (typename THashMap<K, V>::const_iterator it = s1.begin(); it != s1.end(); ++it) {
        typename THashMap<K, V>::const_iterator it2 = s2.find(it->first);
        if (it2 == s2.end() || it2->second != it->second)
            return false;
    }
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMatchable - matches regexs and wordings to it's id
//    Note: add ids in ascending order
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TMatchable {
    TWtrokaAndIds SimplePatterns;
    TRxAndIds RxPatterns;

public:
    bool IsMatch(const TUtf16String& what, int* id = nullptr) const;
    int operator& (IBinSaver& f);
    void AddSimple(const TUtf16String& templ, const int id = 0);
    void AddRx(const TUtf16String& templ, const int id = 0);

    bool operator == (const TMatchable& rhs) const;
};
