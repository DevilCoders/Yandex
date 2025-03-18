#pragma once

#include "compbittrie.h"

#include <util/digest/city.h>
#include <utility>

#include <util/stream/file.h>

#include <util/memory/blob.h>
#include <util/string/cast.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>

template <typename TData>
class TSimhashIndexMemory: private TNonCopyable {
public:
    void MakeFromVector(const TVector<std::pair<ui64, TData>>& sortedData) {
        Bittrie.Clear();
        Data.clear();

        THashMap<TData, ui32> dataOffsets;

        ui64 prevSimhash = 0;
        ui32 groupSize = 0;
        ui32 groupStart = 0;
        for (ui32 i = 0; i < sortedData.size(); ++i) {
            if (i == 0) {
                prevSimhash = sortedData[i].first;
                groupSize = 1;
                groupStart = i;
                continue;
            }
            if (prevSimhash == sortedData[i].first) {
                ++groupSize;
            } else {
                if (groupSize == 1) {
                    uint128 urlHash = sortedData[groupStart].second;
                    std::pair<typename THashMap<TData, ui32>::iterator, bool> res =
                        dataOffsets.insert(std::make_pair(urlHash, (ui32)Data.size()));
                    if (res.second) {
                        Data.push_back(urlHash);
                    }
                    groupStart = res.first->second;
                } else {
                    ui32 newGroupStart = (ui32)Data.size();
                    for (ui32 j = groupStart; j < groupStart + groupSize; ++j) {
                        uint128 urlHash = sortedData[j].second;
                        dataOffsets[urlHash] = newGroupStart + j;
                        Data.push_back(urlHash);
                    }
                    groupStart = newGroupStart;
                }
                if (!Bittrie.Insert(prevSimhash, std::make_pair(groupStart, groupSize))) {
                    ythrow yexception() << "Simhash duplication";
                }
                prevSimhash = sortedData[i].first;
                groupSize = 1;
                groupStart = i;
            }
        }
        if (groupSize == 1) {
            if (groupStart + 1 != sortedData.size()) {
                ythrow yexception() << "wrong last element";
            }
            uint128 urlHash = sortedData[groupStart].second;
            std::pair<typename THashMap<TData, ui32>::iterator, bool> res =
                dataOffsets.insert(std::make_pair(urlHash, (ui32)Data.size()));
            if (res.second) {
                Data.push_back(urlHash);
            }
            groupStart = res.first->second;
        } else {
            ui32 newGroupStart = (ui32)Data.size();
            for (ui32 j = groupStart; j < groupStart + groupSize; ++j) {
                uint128 urlHash = sortedData[j].second;
                dataOffsets[urlHash] = newGroupStart + j;
                Data.push_back(urlHash);
            }
            groupStart = newGroupStart;
        }
        if (!Bittrie.Insert(prevSimhash, std::make_pair(groupStart, groupSize))) {
            ythrow yexception() << "Simhash duplication";
        }

        if (Bittrie.GetCount() > sortedData.size()) {
            ythrow yexception() << "Sizes mismatch";
        }
    }

    void SaveToFiles(const TString& filename) {
        {
            TFixedBufferFileOutput file(filename + ".index");
            Bittrie.Save(file);
        }
        {
            TFixedBufferFileOutput file(filename + ".data");
            file.Write(Data.data(), Data.size() * sizeof(TData));
        }
    }

private:
    TCompBitTrie<ui64, std::pair<ui32, ui32>> Bittrie;
    TVector<TData> Data;
};

template <typename TSimhash, typename TData>
class TSimhashIndexFile: private TNonCopyable {
public:
    bool IsMapped() const {
        return Bittrie.IsMapped();
    }

    void Open(const TString& filename) {
        DataBlob = TBlob::PrechargedFromFile(filename + ".data");
        if (DataBlob.Size() % sizeof(TData) != 0) {
            ythrow yexception() << "Wrong file size";
        }
        Data = (const TData*)DataBlob.Data();

        Bittrie.Open(filename + ".index");
    }

    void GetNeighbors(
        const TSimhash simhash,
        ui32 distance,
        TVector<std::pair<ui64, TData>>& res,
        ui32 maxSize,
        bool clear = true) const {
        if (Data == nullptr) {
            ythrow yexception() << "Data is NULL";
        }
        if (clear) {
            res.clear();
        }

        TCallback callback(this, res, maxSize);
        Bittrie.GetNeighbors(simhash, distance, callback);
    }

private:
    struct TCallback {
        const TSimhashIndexFile* Index;
        TVector<std::pair<ui64, TData>>& Group;
        ui32 MaxSize;
        THashSet<TData> Found;

        TCallback(const TSimhashIndexFile* index,
                  TVector<std::pair<ui64, TData>>& group,
                  ui32 maxSize)
            : Index(index)
            , Group(group)
            , MaxSize(maxSize)
        {
        }

        bool operator()(ui64 simhash, const std::pair<ui32, ui32>& g) {
            ui32 start = g.first;
            ui32 length = g.second;

            if (start + length > Index->DataBlob.Size() / sizeof(TData)) {
                ythrow yexception() << "Group end overflow: " << start + length << " > " << Index->DataBlob.Size() / sizeof(TData);
            }
            if (length == 0) {
                ythrow yexception() << "Empty group";
            }

            for (ui32 j = 0; j < length; ++j) {
                TData url = Index->Data[start + j];
                if (Found.insert(url).second) {
                    Group.push_back(std::make_pair(simhash, url));
                    if (Group.size() >= MaxSize) {
                        return false;
                    }
                }
            }
            return true;
        }
    };

private:
    TCompBitIndex<TSimhash, std::pair<ui32, ui32>> Bittrie;
    TBlob DataBlob;
    const TData* Data;
};

class TBinarySpaceDivider64: private TNonCopyable {
public:
    TBinarySpaceDivider64() {
        for (size_t i = 64; i < 128; ++i) {
            G.push_back((ui8)i);
        }
        for (ui8 i = 0; i < 64; ++i) {
            C.push_back(HadamardCode(i));
        }
        if (G.size() != 64) {
            ythrow yexception() << "wrong generator";
        }
    }

    ui8 MakeSimhashDivider(const ui64 simhash) const {
        ui32 minDist = PopCount(simhash ^ C[0]);
        ui8 minInd = 0;
        for (ui8 i = 1; i < 64; ++i) {
            ui32 dist = PopCount(simhash ^ C[i]);
            if (minDist > dist) {
                minDist = dist;
                minInd = i;
            }
        }
        return minInd;
    }

    ui8 GetPossibleIndexes(
        const ui64 simhash,
        const ui32 distance,
        ui8 indexes[64]) const {
        std::pair<ui8, ui8> ind[64];
        for (ui8 i = 0; i < 64; ++i) {
            ind[i] = std::pair<ui8, ui8>(PopCount(simhash ^ C[i]), i);
        }
        ::Sort(ind, ind + 64);
        indexes[0] = ind[0].second;
        for (ui8 i = 1; i < 64; ++i) {
            if (ind[0].first + 2 * distance > ind[i].first) {
                return i;
            }
            indexes[i] = ind[i].second;
        }
        return 64;
    }

private:
    ui64 HadamardCode(ui8 x) {
        ui64 res = 0;
        for (size_t i = 0; i < 64; ++i) {
            res <<= 1;
            ui8 p = G[i] & x;
            ui32 s = PopCount(p);
            if (s % 2) {
                res |= 1;
            }
        }
        return res;
    }

private:
    TVector<ui8> G;
    TVector<ui64> C;
};

template <typename TData>
class TSimhashIndexFileBundle: private TNonCopyable {
public:
    void Open(const TString& filename) {
        for (size_t i = 0; i < 64; ++i) {
            SimhashIndexes[i].Open(filename + "." + ToString(i));
        }
    }

    void GetNeighbors(const ui64 simhash,
                      const ui32 distance,
                      const ui32 maxSize,
                      TVector<std::pair<ui64, TData>>& res) const {
        res.clear();

        ui8 indexes[64];
        ui8 s = BinarySpaceDivider.GetPossibleIndexes(simhash, distance, indexes);
        for (ui8 i = 0; i < s; ++i) {
            if (!SimhashIndexes[indexes[i]].IsMapped()) {
                ythrow yexception() << "Index is not mapped";
            }
            SimhashIndexes[indexes[i]].GetNeighbors(simhash, distance, res, false);
            if (res.size() >= maxSize) {
                break;
            }
        }
    }

private:
    TBinarySpaceDivider64 BinarySpaceDivider;
    TSimhashIndexFile<ui64, TData> SimhashIndexes[64];
};
