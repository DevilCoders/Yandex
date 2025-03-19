#pragma once

#include <kernel/indexer/parseddoc/pdstorageconf.h>
#include <ysite/directtext/freqs/freqs.h>
#include <library/cpp/langmask/langmask.h>
#include <util/generic/string.h>

struct TIndexProcessorConfig : public TParsedDocStorageConfig {
    TString Prefix;
    TString WorkDir;
    TString SCluster;
    TString PortionList;
    TString OldIndex;
    TString DatHome;
    TString NewIndex;
    TString PureTrieFile;
    TString PureLangConfigFile;
    TString StopWordFile;
    TString PornoWeightsFile;
    TString Dom2Path;
    TString NameExtractorDataPath;
    TString NumberExtractorCfgPath;
    size_t DocCount;
    size_t MaxMemory;
    bool   StoreIndex;
    bool   FinalMerge;
    bool   MergeOld;
    bool   UseExtProcs;
    bool   UseFreqAnalize;
    TFreqCalculator::TKeySet NeededFreqKeys;
    bool   StoreSegmentatorData;
    bool   NoMorphology;
    bool   UseKCParser;  // works if UseExtProcs is set
    bool   UseDater;     // works if UseExtProcs is set
    bool   IsPrewalrus;  // enable certain logic only in robot merges
    TLangMask DefaultLangMask; // will be set to every doc

    TIndexProcessorConfig()
        : TParsedDocStorageConfig()
        , DocCount(0)
        , MaxMemory(0)
        , StoreIndex(false)
        , FinalMerge(false)
        , MergeOld(false)
        , UseExtProcs(true)
        , UseFreqAnalize(true)
        , StoreSegmentatorData(true)
        , NoMorphology(false)
        , UseKCParser(true)
        , UseDater(true)
        , IsPrewalrus(false)
        , DefaultLangMask(LI_BASIC_LANGUAGES)
    {}
};
