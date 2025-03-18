#pragma once

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/system/defaults.h>

template <class TSomeSharedPtr>
struct TSharedPtrLess {
    bool operator()(const TSomeSharedPtr& x, const TSomeSharedPtr& y) const {
        return (size_t)(void*)x.Get() < (size_t)(void*)y.Get();
    }
};

template <class TFirst, class TSecond, class TFirstLess = TSharedPtrLess<TFirst>, class TSecondLess = TSharedPtrLess<TSecond>>
class TMultivalMap {
private:
    typedef TMap<TFirst, TSet<TSecond, TSecondLess>, TFirstLess> TFirstToSecondMap;
    TFirstToSecondMap FirstToSecondMap;

public:
    typedef TSet<TFirst, TFirstLess> TFirstSet;
    typedef TSet<TSecond, TSecondLess> TSecondSet;
    typedef typename TFirstToSecondMap::iterator TFirstMapIterator;
    typedef typename TFirstToSecondMap::const_iterator TFirstMapConstIterator;
    typedef typename TFirstSet::const_iterator TFirstSetConstIterator;
    typedef typename TSecondSet::const_iterator TSecondSetConstIterator;

    void AddPointToDomain(TFirst first, TSecond second) {
        FirstToSecondMap[first].insert(second);
    }
    void AddPointToDomain(TFirst first, TSecondSet secondSet) {
        if (secondSet.empty())
            return;
        FirstToSecondMap[first].insert(secondSet.begin(), secondSet.end());
    }
    void AddPointToCodomain(TFirstSet firstSet, TSecond second) {
        for (TFirstSetConstIterator it = firstSet.begin(); it != firstSet.end(); it++)
            AddPointToDomain(*it, second);
    }
    void AddSetToDomain(TFirstSet firstSet, TSecondSet secondSet) {
        if (secondSet.empty())
            return;
        for (TFirstSetConstIterator it = firstSet.begin(); it != firstSet.end(); it++)
            AddPointToDomain(*it, secondSet);
    }
    void ImportMap(const TMultivalMap& map) {
        for (TFirstMapConstIterator it = map.FirstToSecondMap.begin(); it != map.FirstToSecondMap.end(); it++)
            FirstToSecondMap[it->first].insert(it->second.begin(), it->second.end());
    }

    TSecondSet GetImage(TFirst first) const {
        TFirstMapConstIterator it = FirstToSecondMap.find(first);
        return (it == FirstToSecondMap.end()) ? TSecondSet() : it->second;
    }
    TSecondSet GetImage(TFirstSet firstSet) const {
        TSecondSet ret;
        for (TFirstSetConstIterator it = firstSet.begin(); it != firstSet.end(); it++) {
            TSecondSet set = GetImage(*it);
            ret.insert(set.begin(), set.end());
        }
        return ret;
    }
    TFirstSet GetDomain() const {
        TFirstSet ret;
        for (TFirstMapConstIterator it = FirstToSecondMap.begin(); it != FirstToSecondMap.end(); it++)
            ret.insert(it->first);
        return ret;
    }

    void RemoveSetFromDomain(TFirstSet firstSet) {
        for (TFirstSetConstIterator it = firstSet.begin(); it != firstSet.end(); it++)
            FirstToSecondMap.erase(*it);
    }
    void RemovePairs(TFirstSet firstSet, TSecondSet secondSet) {
        for (TFirstSetConstIterator it = firstSet.begin(); it != firstSet.end(); it++) {
            TFirstMapIterator f2s = FirstToSecondMap.find(*it);
            if (f2s == FirstToSecondMap.end())
                continue;
            for (TSecondSetConstIterator sit = secondSet.begin(); sit != secondSet.end(); sit++)
                f2s->second.erase(*sit);
            if (f2s->second.empty())
                FirstToSecondMap.erase(f2s);
        }
    }
};

template <class TFirst, class TSecond, class TFirstLess = TSharedPtrLess<TFirst>, class TSecondLess = TSharedPtrLess<TSecond>>
class TBidirectMultivalMap {
private:
    typedef TMultivalMap<TFirst, TSecond, TFirstLess, TSecondLess> TDirectMap;
    typedef TMultivalMap<TSecond, TFirst, TSecondLess, TFirstLess> TInverseMap;
    TDirectMap DirectMap;
    TInverseMap InverseMap;

public:
    typedef TSet<TFirst, TFirstLess> TFirstSet;
    typedef TSet<TSecond, TSecondLess> TSecondSet;

    void AddPairs(TFirstSet firstSet, TSecondSet secondSet) {
        DirectMap.AddSetToDomain(firstSet, secondSet);
        InverseMap.AddSetToDomain(secondSet, firstSet);
    }
    void AddPairs(TFirst first, TSecondSet secondSet) {
        DirectMap.AddPointToDomain(first, secondSet);
        InverseMap.AddPointToCodomain(secondSet, first);
    }
    void AddPairs(TFirstSet firstSet, TSecond second) {
        DirectMap.AddPointToCodomain(firstSet, second);
        InverseMap.AddPointToDomain(second, firstSet);
    }
    void AddPairs(TFirst first, TSecond second) {
        DirectMap.AddPointToDomain(first, second);
        InverseMap.AddPointToDomain(second, first);
    }
    void ImportMap(const TBidirectMultivalMap& map) {
        DirectMap.ImportMap(map.DirectMap);
        InverseMap.ImportMap(map.InverseMap);
    }

    TSecondSet GetImage(TFirstSet firstSet) const {
        return DirectMap.GetImage(firstSet);
    }
    TSecondSet GetImage(TFirst first) const {
        return DirectMap.GetImage(first);
    }
    TFirstSet GetCoimage(TSecondSet secondSet) const {
        return InverseMap.GetImage(secondSet);
    }
    TFirstSet GetCoimage(TSecond second) const {
        return InverseMap.GetImage(second);
    }

    TFirstSet GetDomain() const {
        return DirectMap.GetDomain();
    }
    TSecondSet GetCodomain() const {
        return InverseMap.GetDomain();
    }

    void RemoveSetFromDomain(TFirstSet firstSet) {
        TSecondSet image = DirectMap.GetImage(firstSet);
        DirectMap.RemoveSetFromDomain(firstSet);
        InverseMap.RemovePairs(image, firstSet);
    }
    void RemoveSetFromCodomain(TSecondSet secondSet) {
        TFirstSet coimage = InverseMap.GetImage(secondSet);
        InverseMap.RemoveSetFromDomain(secondSet);
        DirectMap.RemovePairs(coimage, secondSet);
    }
};
