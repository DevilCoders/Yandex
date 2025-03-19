#pragma once

#include <util/string/vector.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>

//lems collector (all word must have lemma for search)
namespace NIndexerCore {
    struct TDirectTextData2;
}

class IWordDetector;

class TLemmasCollector  {
private:
    struct TLemmaInfoAndDetector {
        IWordDetector* Detector;
        TString ExactForm;

        TLemmaInfoAndDetector(IWordDetector* detector, const TString& exactForm)
            : Detector(detector)
            , ExactForm(exactForm)
        {}
    };

    typedef TVector<TLemmaInfoAndDetector> TDetectorList;
    typedef THashMap<TString, TDetectorList> TLemmasDict;
    TLemmasDict Dict;
    typedef TLemmasDict::iterator TIterator;
private:
    void Detect(ui32 position, const char* lemma, const TWtringBuf& token);
public:
    void AddLemmaAndDetector(const TString& lemma, IWordDetector* detector, const TString& exactForm);
    void AddLemmaAndDetector(const TString& lemma, IWordDetector* detector);
    void Process(const NIndexerCore::TDirectTextData2& directText);
};
