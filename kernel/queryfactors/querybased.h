#pragma once

#include "detectors.h"
#include "nwords_detector.h"
#include "lemmas_collector.h"

#include <kernel/indexer/face/dtproc.h>
#include <kernel/indexer/direct_text/dt.h>

#include <util/string/vector.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>

const ui32 DEFAULT_DOC_LEN = 1024;
const ui32 DEFAULT_TITLE_LEN = 10;
const float AdultEps = 1e-8f;


// get multipliers for 2 parts of Adultness
void GetRelevZoneMultipliers(enum RelevLevel zone, float& multiplier1, float& multiplier2);

class TQueryFactorsConfig {
private:
    TString ConfigDir;
public:
    bool UsePrepared; // use prepared files, without call CreateRichTree and ParseRichTree

    TString CommFileName;
    TString PaymentsFileName;
    TString SeoFileName;
    TString SeoFileNameExact;
    TString PornoFileName;
    TString PornoFilePairs;
    TString AdultFileName;

    TQueryFactorsConfig(const TString& configDir, bool usePrepared = false)
        : UsePrepared(usePrepared)
    {
        SetConfigDir(configDir);
    }

    TQueryFactorsConfig()
        : UsePrepared(false)
    {}

    void SetConfigDir(const TString& configDir) {
        ConfigDir = configDir;
        CommFileName =  configDir + "/comm.query";
        PaymentsFileName = configDir + "/payments.dict";
        SeoFileName = configDir + "/seo.query";
        PornoFileName = configDir + "/porno.query";
        PornoFilePairs = configDir + "/porno.query.pairs";
        AdultFileName = configDir + "/porno_config.dat";
        SeoFileNameExact = SeoFileName + ".exact";
    }

    TString GetConfigDir() const {
        return ConfigDir;
    }
};

class TQueryBasedProcessor : public IDirectTextProcessor {
private:
    TLemmasCollector LemmasCollector;

    THolder<IWordDetector> CommercialDetector;
    THolder<IWordDetector> PaymentsDetector;

    THolder<IWordDetector> SeoDetector;
    THolder<IWordDetector> SeoExactDetector;

    THolder<IWordDetector> PornoDetector;
    THolder<TDetectorUnion> Porno2WordsDetector;

    THolder<TAdultDetector> AdultDetector;

public:
    TQueryBasedProcessor(const TQueryFactorsConfig& config);
    void InitAdultDetector(const TString& adultFileName, TAdultPreparat& adultPreparat, bool usePrepared);
    void ProcessDirectText2(IDocumentDataInserter* inserter, const NIndexerCore::TDirectTextData2& directText, ui32 docId) override;
private:
    void Prepare();
};

IWordDetector* BuildSimpleDetector(const TString& queryFileName, TLemmasCollector& collector, bool findExactWords = false, bool findInTitle = false, bool usePrepared = false);
IWordDetector* BuildPaymentsDetector(const TString& queryFileName, TLemmasCollector& collector);
void BuildPornoDetectors(const TString& queryFileName, const TString& queryFileNamePairs, TLemmasCollector& collector, THolder<IWordDetector>& simpleDetector,  THolder<TDetectorUnion>& pairDetector, bool usePrepared);

TAutoPtr<TAdultDetector> BuildAdultDetector(const TString& pornoConfig, TAdultPreparat& adultPreparat, TLemmasCollector& collector, bool usePrepared = false);
void GetRelevZoneMultipliers(enum RelevLevel zone, float& multiplier1, float& multiplier2);
// for porno_config parsing (use in erf_create)
float GetPornoThreshold(const TString& line);
void ParsePornoConfigLine(const TString line, TVector<TString>& words, TString& query, float& weight);

// for query prepare
void MakeWordsFromRichTree(const TString& queryFileName, TVector<TString>& words, bool findExactWords, bool findInTitle);
void ConvertWords2Lemmas(const TVector<TString>& words, TVector<TString>& textLemmas);
void MakeWordsAndLemmas4PornoDetectors(const TString& queryFileName, NQueryFactors::TQueriesInfo& queriesInfo, TVector<TString>& lemmas);
bool GetAdultLemmasFromString(const TString& line, float& wordWeight, TVector<TString>& lemmas);
