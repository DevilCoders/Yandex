#pragma once

#include "detectors.h" // for IWordDetector
#include "lemmas_collector.h" // for TLemmasCollector
#include <kernel/indexer/direct_text/dt.h>

#include <util/string/vector.h> // for TVector<TString>

struct TAdultPreparat {
    float AdultFree;
    float AdultDepend;
    float AdultThreshold;

    void Prepare() {
        AdultFree = 0.0;
        AdultDepend = 0.0;
    }
};

namespace NQueryFactors {

    struct TQueryWordInfo {
        const TString Word;
        const ui32 Distance;

        TQueryWordInfo(const TString& word, ui32 distance)
            : Word(word)
            , Distance(distance)
        { }
};

    typedef TVector<TQueryWordInfo> TQueryInfo;
    typedef TVector<TQueryInfo> TQueriesInfo;

}

// can process several words in query
template <class TOneWordDetector = TPositionDetector>
class TNWordsDetector {
protected:
    TVector<TSimpleSharedPtr<TOneWordDetector> > DetectorsList;

public:
    typedef typename TOneWordDetector::TFactory TFactory;

    TNWordsDetector() {
    }

    void Init(const NQueryFactors::TQueryInfo& queryInfo, TLemmasCollector& lemmasCollector, TFactory& factory) {
        Y_VERIFY(queryInfo.size(), "Number of words for TNWordsDetector must be more than 0.");

        TOneWordDetector* prevDetector = nullptr;
        for (size_t i = 0; i < queryInfo.size(); ++i) {
            const NQueryFactors::TQueryWordInfo& wordInfo = queryInfo[i];
            THolder<TOneWordDetector> detector(factory.CreateDetector(prevDetector, wordInfo.Distance));
            lemmasCollector.AddLemmaAndDetector(wordInfo.Word, detector.Get());
            prevDetector = detector.Get();
            DetectorsList.push_back(detector.Release());
        }
        prevDetector->SetLast();
    }

    void Prepare() {
        Y_ASSERT(DetectorsList.size());
        for (size_t i = 0; i < DetectorsList.size(); ++i)
            DetectorsList[i]->Prepare();
    }

    bool GetResult() {
        Y_ASSERT(DetectorsList.size());
        return DetectorsList.back()->GetResult();
    }

    ui32 GetLastPos() {
        return DetectorsList.back()->GetWordPos();
    }

};

// set of several TNWordsDetector for complex processing boolean factors
class TDetectorUnion {
private:
    typedef TNWordsDetector<> TDetector;
    TVector<TSimpleSharedPtr<TDetector> > DetectorsList;

public:

    TDetectorUnion(const NQueryFactors::TQueriesInfo& queries, TLemmasCollector& collector);
    void Prepare();
    bool GetResult();
};

// calc adultness
class TAdultDetector {
private:
    class TDetectorWithWeight;

    class T2CaseDetector : public TPositionDetector {
    private:
        TDetectorWithWeight& Owner;

    protected:
        T2CaseDetector(TDetectorWithWeight& owner, T2CaseDetector* prevDetector = nullptr, ui32 distance = 1)
            : TPositionDetector(prevDetector, distance)
            , Owner(owner)
        { }

    public:
        void Detect(ui32 curWordPos, const TString& lemma) override;

        class TFactory {
        private:
            TDetectorWithWeight& Owner;
        public:
            TFactory(TDetectorWithWeight& owner)
                : Owner(owner)
            { }

            T2CaseDetector* CreateDetector(T2CaseDetector* prevDetector, ui32 distance) {
                return new T2CaseDetector(Owner, prevDetector, distance);
            }
        };

    };

    class TDetectorWithWeight : public  TNWordsDetector<T2CaseDetector> {
    private:
        typedef TNWordsDetector<T2CaseDetector> TBase;

        float Weight;
        TAdultPreparat& AdultPreparat;

    public:
        TDetectorWithWeight(float weight, TAdultPreparat& adultPreparat);

        inline float GetWeight() const {
            return Weight;
        }

        inline TAdultPreparat& GetAdultPreparat() {
            return AdultPreparat;
        }
    };

    // one TNWordsDetector for one query and weight
    TVector<TSimpleSharedPtr<TDetectorWithWeight> > DetectorsStorage;
    TAdultPreparat& AdultPreparat;

public:
    TAdultDetector(TAdultPreparat& adultPreparat)
        : AdultPreparat(adultPreparat)
    { }

    void AddWordsSequence2Detect(const TVector<TString>& lemmas, float weight, TLemmasCollector& lemmasCollector);
    void Prepare();
};
