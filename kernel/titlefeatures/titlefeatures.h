#pragma once

#include <kernel/indexer/face/dtproc.h>
#include <kernel/indexer/direct_text/dt.h>
#include <ysite/relevance_tools/subphrase_finder_file.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>

struct TTitleWordInfo;

////////////////////////
// For prewalrus
////////////////////////

struct TTitleFeaturesConfig {
    TString PureTrieFile;

    TTitleFeaturesConfig()
        : PureTrieFile("")
    {
    }

    TTitleFeaturesConfig(const TString& pureTrieFile)
        : PureTrieFile(pureTrieFile)
    {
    }
};

class TTitleFeaturesProcessor : public IDirectTextProcessor {
private:
    ui64 PureCollectionLength;
public:
    TTitleFeaturesProcessor(const TTitleFeaturesConfig& config);
    void ProcessDirectText2(IDocumentDataInserter* inserter, const NIndexerCore::TDirectTextData2& directText, ui32 docId) override;
};


////////////////////////
// For erfcreate
////////////////////////

class TTitleLRBM25Calculator {
private:
    typedef THashMap<TString, TVector<size_t> > TTitleDict;

    TVector<TTitleWordInfo> WordInfos;
    TTitleDict ExactForms;
    ui32 DocLen;
    ui32 NumHits;

private:
    void AddTitleWord(const TString& word);
    void DetectHits(const TString& word);

public:
    TTitleLRBM25Calculator();
    ~TTitleLRBM25Calculator();

    void SetTitle(const TString& title);
    void Feed(const TString& text);
    float GetFactor() const;
};
