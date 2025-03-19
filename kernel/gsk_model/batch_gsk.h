#pragma once
#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/containers/hash_trie/hash_trie.h>

struct TBatchRegexpCalcer
{
    enum {
        BINARY_FEATURE = 0x80000000
    };
    struct TBinaryFeatureStat
    {
        int SrcIdx;
        float Border;

        SAVELOAD(SrcIdx, Border)

        TBinaryFeatureStat() : SrcIdx(0), Border(0) {}
        TBinaryFeatureStat(int srcIdx, float border) : SrcIdx(srcIdx), Border(border) {}
    };
    struct TStateFlipInfo
    {
        ui16 RegexpId;
        union {
            struct {
                ui8 Unused;
                ui8 SrcState;
            };
            ui16 SrcStateHack;
        };

        TStateFlipInfo() : RegexpId(0), SrcStateHack(0) {}
    };
    struct TTree
    {
        TVector<int> FeatureIds;
        TVector<int> BinValues;

        SAVELOAD(FeatureIds, BinValues)
    };

    enum EWordTransitionPtr {
        NEXT_PTR = 0,
        START_NEXT_PTR = 1,
        FINISH_NEXT_PTR = 2,
        START_FINISH_NEXT_PTR = 3,
    };

    TVector<TStateFlipInfo> AllStateFlips;
    // WordTransitions[wordId * 4 + EWordTransitionPtr] - pointer to the array in AllStateFlips
    TVector<int> WordTransitions;
    TVector<ui8> AcceptStates;
    TSimpleHashTrie Words;
    TVector<TTree> Model;
    TVector<TBinaryFeatureStat> BinaryFeatures;
    double ResultScale;

    SAVELOAD(AllStateFlips, WordTransitions, AcceptStates, Words, Model, BinaryFeatures, ResultScale)
};

double ComputeModel(const TBatchRegexpCalcer &data, const TVector<int> &str, const float *fFactors);
