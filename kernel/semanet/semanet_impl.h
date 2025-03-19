#pragma once
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/packers/packers.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <library/cpp/langmask/langmask.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class TRecord>
class TRegionPacker {
public:
    typedef TArrayRef<TRecord> TRecords;

    void UnpackLeaf(const char *p, TRecords& result) const {
        size_t len;
        NPackers::TIntegralPacker<size_t>().UnpackLeaf(p, len);
        size_t start = NPackers::TIntegralPacker<size_t>().SkipLeaf(p);
        result = TRecords((TRecord*)(p + start), len);
    }

    void PackLeaf(char* buf, const TRecords& data, size_t computedSize) const {
        size_t len = data.size();
        size_t lenChar = len * sizeof(TRecord);
        size_t start = computedSize - lenChar;
        NPackers::TIntegralPacker<size_t>().PackLeaf(buf, len, NPackers::TIntegralPacker<size_t>().MeasureLeaf(len));
        memcpy(buf + start, data.data(), lenChar);
    }

    size_t MeasureLeaf(const TRecords& data) const {
        size_t len = data.size();
        return NPackers::TIntegralPacker<size_t>().MeasureLeaf(len) + len * sizeof(TRecord);
    }

    size_t SkipLeaf(const char *p) const {
        size_t result = NPackers::TIntegralPacker<size_t>().SkipLeaf(p);
        size_t len;
        NPackers::TIntegralPacker<size_t>().UnpackLeaf(p, len);
        result += len * sizeof(TRecord);
        return result;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSemanetRecord {
    ui32 TargetId;
    float Probability;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef TArrayRef<TSemanetRecord> TSemanetRecords;
typedef TRegionPacker<TSemanetRecord> TSemanetRecordsPacker;
typedef TCompactTrie<char, TSemanetRecords, TSemanetRecordsPacker> TSemanetSTrie;
typedef TCompactTrie<wchar16, TSemanetRecords, TSemanetRecordsPacker> TSemanetWTrie;
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T, class TWeight>
class TMostFrequentTracker {
    TMap<T, TWeight> Weights;
    struct TElementAndWeight {
        T Element;
        TWeight Weight;

        TElementAndWeight(const T &e, TWeight w)
            : Element(e)
            , Weight(w) {
        }

        bool operator < (const TElementAndWeight &rhs) const {
            if (Weight < rhs.Weight)
                return false;
            if (rhs.Weight < Weight)
                return true;
            return Element < rhs.Element;
        }
    };
    TSet<TElementAndWeight> Tracker;
public:
    void Inc(const T &element, TWeight weight) {
        typename TMap<T, TWeight>::iterator it = Weights.find(element);
        if (it != Weights.end()) {
            Tracker.erase(TElementAndWeight(it->first, it->second));
            it->second += weight;
            Tracker.insert(TElementAndWeight(it->first, it->second));
        } else {
            Tracker.insert(TElementAndWeight(element, weight));
            Weights[element] = weight;
        }
    }
    void Remove(const T &element) {
        typename TMap<T, TWeight>::iterator it = Weights.find(element);
        if (it != Weights.end()) {
            Tracker.erase(TElementAndWeight(it->first, it->second));
            Weights.erase(it);
        }
    }
    const T& GetMostFrequent(TWeight *weight = nullptr) const {
        const TElementAndWeight &res = *Tracker.begin();
        if (weight)
            *weight = res.Weight;
        return res.Element;
    }
    bool Empty() const {
        return Weights.empty();
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class TSemanetTracker: public TMostFrequentTracker<T, float> {
public:
    THashSet<T> Done;
    TVector<T> Mined;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void TrackerToResult(TSemanetTracker<TUtf16String> *tracker, TVector<TUtf16String> *res, size_t count);
////////////////////////////////////////////////////////////////////////////////////////////////////
struct TSemanetTrackers {
    TSemanetTracker<TUtf16String> Queries;
    TSemanetTracker<TString> Hosts;
    TSemanetTracker<TString> Urls;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EAddMode {
    ASK_USER,
    ADD_ALWAYS,
    ADD_NEVER
};
