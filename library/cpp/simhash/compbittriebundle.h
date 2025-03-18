#pragma once

#include "compbittrie.h"

#include <util/generic/ptr.h>

class THadamardCode: private TNonCopyable {
public:
    struct TVectorDistance {
        ui64 Difference;
        ui32 Distance;
        ui8 VectorIndex;

        TVectorDistance()
            : Difference(0)
            , Distance(0)
            , VectorIndex(0)
        {
        }

        bool operator<(const TVectorDistance& o) const {
            if (Distance == o.Distance) {
                return VectorIndex < o.VectorIndex;
            } else {
                return Distance < o.Distance;
            }
        }
    };

    THadamardCode() {
        for (ui8 i = 64; i < 128; ++i) {
            G[i - 64] = i;
        }
        for (ui64 i = 0; i < 128; ++i) {
            C[i] = Code(i);
        }
        for (ui64 i = 0; i < 128; ++i) {
            for (ui64 j = 0; j < 128; ++j) {
                CC[i][j] = std::pair<ui64, ui32>(C[i] ^ C[j],
                                                 PopCount(C[i] ^ C[j]));
            }
        }
    }

    ui64 Code(ui8 x) const {
        if (x >= 128) {
            ythrow yexception() << "x must be in [0, 128)";
        }
        ui64 res = 0;
        for (size_t i = 0; i < 64; ++i) {
            res <<= 1;
            ui8 p = G[i] & x;
            ui32 s = PopCount(p);
            res |= (s % 2);
        }
        return res;
    }

    void CalculateDistances(
        const ui64 simhash,
        const ui8 vectorsToUse,
        TVector<std::pair<ui64, ui32>>* distances,
        ui8* minIndex) const {
        if (vectorsToUse == 0) {
            ythrow yexception() << "vectorsToUse must be > 0";
        }
        if (vectorsToUse > 128) {
            ythrow yexception() << "128 vectors total. vectorsToUse = " << (ui16)vectorsToUse;
        }

        ui32 minDist = PopCount(simhash ^ C[0]);
        ui8 minInd = 0;
        if (distances != nullptr) {
            distances->resize(vectorsToUse);
            (*distances)[0].first = simhash ^ C[0];
            (*distances)[0].second = minDist;
        }
        for (ui8 i = 1; i < vectorsToUse; ++i) {
            ui64 x = simhash ^ C[i];
            ui32 dist = PopCount(x);
            if (minDist > dist) {
                minDist = dist;
                minInd = i;
            }
            if (distances != nullptr) {
                (*distances)[i].first = x;
                (*distances)[i].second = dist;
            }
        }
        *minIndex = minInd;
    }

    void CalculateDistancesSorted(
        const ui64 simhash,
        const ui8 vectorsToUse,
        TVector<TVectorDistance>* distances) const {
        if (vectorsToUse == 0) {
            ythrow yexception() << "vectorsToUse must be > 0";
        }
        if (vectorsToUse > 128) {
            ythrow yexception() << "128 vectors total. vectorsToUse = " << (ui16)vectorsToUse;
        }

        distances->resize(vectorsToUse);
        for (ui8 i = 0; i < vectorsToUse; ++i) {
            (*distances)[i].VectorIndex = i;
            (*distances)[i].Difference = simhash ^ C[0];
            (*distances)[i].Distance = PopCount((*distances)[i].Difference);
        }
        ::Sort(*distances);
    }

    ui8 MakeSimhashDivider(const ui64 simhash, const ui8 vectorsToUse) const {
        ui8 minInd = 0;
        CalculateDistances(simhash, vectorsToUse, nullptr, &minInd);
        return minInd;
    }

    ui64 GetVector(ui8 n) {
        if (n >= 128) {
            ythrow yexception() << "128 vectors total. n = " << (ui16)n;
        }
        return C[n];
    }

private:
    ui8 G[64];
    ui64 C[128];
    std::pair<ui64, ui32> CC[128][128];
};

class THadamardCodeBitTrieHints {
public:
    THadamardCodeBitTrieHints(
        const ui64 simhash,
        const ui32 searchDistance,
        const ui32 distanceDifference,
        const ui64 xORWithNearest,
        const ui64 xORWithCurrentButNotNearest)
        : Simhash(simhash)
        , SearchDistance(searchDistance)
        , DistanceDifference(distanceDifference)
        , XORWithNearest(xORWithNearest)
        , XORWithCurrentButNotNearest(xORWithCurrentButNotNearest)
    {
    }

    bool IsPrefixAllowed(
        ui64 nodeSimhash,
        ui8 nodeDividingBit) const {
        if (nodeDividingBit > 64) {
            ythrow yexception() << "nodeDividingBit > 64";
        }
        if (nodeDividingBit == 0) {
            return true;
        }
        nodeSimhash >>= 64 - nodeDividingBit;
        const ui64 nodeXOR = (nodeSimhash ^ (Simhash >> (64 - nodeDividingBit)))
                             << (64 - nodeDividingBit);
        const ui32 nodeXORpop = PopCount(nodeXOR);
        if (nodeXORpop > SearchDistance) {
            ythrow yexception() << "nodeXORpop > SearchDistance";
        }
        const i32 proximityToNearest = (i32)PopCount(XORWithNearest & nodeXOR);
        const i32 proximityToCurrent = (i32)PopCount(XORWithCurrentButNotNearest & nodeXOR);
        if (DistanceDifference + proximityToNearest > SearchDistance + proximityToCurrent - nodeXORpop) {
            return false;
        }
        return true;
    }

private:
    const ui64 Simhash;
    const ui32 SearchDistance;
    const ui32 DistanceDifference;
    const ui64 XORWithNearest;
    const ui64 XORWithCurrentButNotNearest;
};

template <typename TValueType,
          template <typename> class TSimhashTraits = ::TSimhashTraits>
class TCompBitTrieBundle {
public:
    typedef TCompBitTrie<ui64, TValueType, TSimhashTraits> TBitTrie;

    explicit TCompBitTrieBundle(ui8 bucketCount = 64)
        : BucketCount(bucketCount)
    {
        Tries.Reset(new TBitTrie[BucketCount]);
    }

public:
    void Clear() {
        for (ui8 i = 0; i < BucketCount; ++i) {
            Tries[i].Clear();
        }
    }

    bool Insert(
        const ui64 simhash,
        const TValueType& value,
        TValueType** oldValue = NULL) {
        ui8 div = Divider.MakeSimhashDivider(simhash, BucketCount);
        if (div >= BucketCount) {
            ythrow yexception() << "wrong divider";
        }
        return Tries[div].Insert(simhash, value, oldValue);
    }

    bool Delete(const ui64 simhash, TValueType* value = NULL) {
        ui8 div = Divider.MakeSimhashDivider(simhash, BucketCount);
        if (div >= BucketCount) {
            ythrow yexception() << "wrong divider";
        }
        return Tries[div].Delete(simhash, value);
    }

    const TValueType& Get(const ui64 simhash) const {
        ui8 div = Divider.MakeSimhashDivider(simhash, BucketCount);
        if (div >= BucketCount) {
            ythrow yexception() << "wrong divider";
        }
        return Tries[div].Get(simhash);
    }

    template <class TRes>
    void GetNeighbors(
        const ui64 simhash,
        ui32 distance,
        TVector<TRes>* simhashes) const {
        simhashes->clear();

        Divider.CalculateDistancesSorted(
            simhash,
            BucketCount,
            &Distances);
        ui32 minDist = Distances[0].Distance;

        Tries[0].GetNeighbors(simhash, distance, simhashes, false);
        simhashes->reserve(simhashes->size() * 3);

        for (ui8 i = 1; i < BucketCount; ++i) {
            const ui32 dist = Distances[i].Distance;
            if ((dist < distance) || (dist - distance <= minDist + distance)) {
                THadamardCodeBitTrieHints hints(
                    simhash,
                    distance,
                    Distances[i].Distance - Distances[0].Distance,
                    Distances[0].Difference,
                    Distances[i].Difference);
                Tries[Distances[i].VectorIndex].GetNeighborsWithHints(
                    simhash,
                    distance,
                    simhashes,
                    false,
                    hints);
            } else {
                break;
            }
        }
    }

private:
    const ui8 BucketCount;
    TArrayHolder<TBitTrie> Tries;
    THadamardCode Divider;

    //Temp variables
    mutable TVector<THadamardCode::TVectorDistance> Distances;
};
