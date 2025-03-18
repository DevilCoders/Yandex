#pragma once

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

struct TBlock {
    enum EBlockType { SIMPLE_FACTOR, EXPR_FACTOR, USER_BLOCK, STRUCTURE_BLOCK };

    // parsed from input:

    TString Id;                   // block's name
    EBlockType Type;             // what kind of SFDL statement described this block
    TVector<TString> Deps;        // names of blocks, structures, fields and factors on which block depends
    TVector<TString> Factors;     // names of factors which block computes
    TString Code;                 // C++ code

    // computed in Enrich()
    size_t Index;                // index in Blocks[]
    TVector<int> FactorIndexes;  // sorted list of indexed of computed factors
    TVector<TBlock*> DepBlocks;  // all resolved block dependencies including transitive
    TVector<int> ChildFactorIndexes;

    // code generator variables
    int Rank;
    TVector<int> RequiredFactorIndexes;
    bool Required;
    bool Emitted;

    TBlock()
        : Type(SIMPLE_FACTOR)
        , Index(-1)
        , Rank(-1)
        , Required(false)
        , Emitted(false)
    {
    }
};

struct TState {
    TString InputFile;

    TVector<TBlock*> Blocks;
    THashMap<TString, TBlock*> Name2Block;

    TString ArgList;
    TVector<TString> FastRankFiles;
    TVector<TString> FastRankFactors;
    TVector<TString> ExternalFactors;

    TVector<int> FastRankFactorsIndexes;
    THashSet<int> ExternalFactorsIndexesHash;

    THashMap<int, TBlock*> Factor2Block;
    TVector<int> AvailableFactorsIndexes;

    bool IsFactorAvailable(int factor) const {
        return Factor2Block.contains(factor);
    }

    void AddBlock(TBlock* block);
    TBlock* FindBlock(const TString& name);
    TBlock* GetBlock(const TString& name);    // aborts on failure

    ~TState();
};

void ScanFile(const TString& filename);
TState* ParseFile(const TString& filename);
