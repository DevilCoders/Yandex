#pragma once
#include <kernel/lingboost/enum_map.h>
#include <kernel/lingboost/constants.h>

namespace NTextMachine {

struct TDataExtractorType {
    enum EType {
        Unknown = 0,
        Example = 1,
        DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19 = 2, //FACTOR-2031
        CountersForDssmSSHardWordWeights = 3,
        FullSplitBertCounters = 4,
        MAX
    };
    static const size_t Size = MAX;

    static TArrayRef<const EType> GetValuesRegion() {
        static const EType values[] = {
            Unknown,
            Example,
            DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19,
            CountersForDssmSSHardWordWeights,
            FullSplitBertCounters,
        };
        return {values, Y_ARRAY_SIZE(values)};
    }
};

using EDataExtractorType = TDataExtractorType::EType;

class IDataExtractorBase {
public:
    virtual ~IDataExtractorBase() {}
    virtual EDataExtractorType GetType() const = 0;
};

template<EDataExtractorType Type>
class IDataExtractorImpl;


using TDataExtractorsList = TVector<IDataExtractorBase*>;

template<EDataExtractorType Type>
using TTypedDataExtractorsList = TVector<IDataExtractorImpl<Type>*>;

using TSimpleExtractorsRegistry = NLingBoost::TStaticEnumMap<TDataExtractorType, TDataExtractorsList>;



template<>
class IDataExtractorImpl<EDataExtractorType::Example> : public IDataExtractorBase {
public:
    IDataExtractorImpl() {}

    EDataExtractorType GetType() const override {
        return EDataExtractorType::Example;
    }

    virtual void Kuku() const = 0; // User function, that can be implemented by concrete unit
};

struct TQuery;

template<>
class IDataExtractorImpl<EDataExtractorType::DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19>
    : public IDataExtractorBase
{
public:
    struct TWordCounters {
        size_t TermFrequencyAny = 0;
        size_t TermFrequencyExact = 0;

        size_t MaxQueryWordsInOneSentenceFoundWithCurrent = 0;
        size_t QueryWordsFoundWithCurrent = 0;
        float AvgMaxInverseDistance = 0;
    };

    struct TBigramCounters {
        size_t SentecesWithBothWords = 0;
        size_t SentencesWithBothWordsAnyFormNeighboring = 0;
        size_t SentencesWithBothWordsExactNeighboring = 0;
        size_t MaxQueryWordsInOneSentenceFoundWithCurrentWords = 0;
        size_t MaxQueryWordsInOneSentenceFoundWithCurrentWordsNeighboring = 0;

        float InverseDistanceSum = 0;
    };

    struct TStreamCounters {
        TVector<TWordCounters> WordCounters;
        TVector<TBigramCounters> BigramCounters;
    };

    struct TQueryStats {
        const NTextMachine::TQuery* Query = nullptr;

        TStreamCounters CountersByUrl;
        TStreamCounters CountersByTitle;
        TStreamCounters CountersByBody;
    };

    virtual TQueryStats GetQueryStats() = 0;

    EDataExtractorType GetType() const final {
        return EDataExtractorType::DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19;
    }
};

using TForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19 =
    IDataExtractorImpl<EDataExtractorType::DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19>::TQueryStats;


template<>
class IDataExtractorImpl<EDataExtractorType::CountersForDssmSSHardWordWeights>
    : public IDataExtractorBase
{
public:
    struct TWordCounters {
        bool FoundInTitle = false;
        bool FoundInQueryDwellTimeStream = false; //TODO(ilnurkh) delete this, not needed any more
    };

    struct TQueryStats {
        const NTextMachine::TQuery* Query = nullptr;

        TVector<TWordCounters> WordCounters;
    };

    virtual TQueryStats GetQueryWordsOccuranceStats() = 0;

    EDataExtractorType GetType() const final {
        return EDataExtractorType::CountersForDssmSSHardWordWeights;
    }
};

template<>
class IDataExtractorImpl<EDataExtractorType::FullSplitBertCounters>
    : public IDataExtractorBase
{
public:
    struct TCounters {
        size_t ExactHitsInBody = 0;
        float Idfs = 0;
    };

    TVector<TCounters> Counters;

    virtual TVector<TCounters> GetCounters() = 0;

    EDataExtractorType GetType() const final {
        return EDataExtractorType::FullSplitBertCounters;
    }
};

using TCountersForDssmSSHardWordWeights =
    IDataExtractorImpl<EDataExtractorType::CountersForDssmSSHardWordWeights>::TQueryStats;

using TFullSplitBertCounters = 
    TVector<IDataExtractorImpl<EDataExtractorType::FullSplitBertCounters>::TCounters>;

} //namespace NTextMachine
