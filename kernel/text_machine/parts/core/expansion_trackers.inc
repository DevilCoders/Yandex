#define UNIT_EXPANSION_MULTI_TRACKER(UnitNamePrefix, Expansion, Mode) \
    MACHINE_PARTS(Core) { \
        using UnitNamePrefix##Group = TMultiTrackerGroup<Expansion>; \
        using UnitNamePrefix##Family = UnitNamePrefix##Group::TMultiTrackerFamily; \
        template <typename TrackerType> \
        using UnitNamePrefix##Stub = UnitNamePrefix##Group::TMultiTrackerStub<TrackerType, Mode>; \
        \
        UNIT_FAMILY_REGISTER( \
            Core, \
            UnitNamePrefix ## Family, \
            TMultiTrackerFamilyInfo, Expansion, Mode); \
    }

#define UNIT_EXPANSION_AGGREGATOR(AggNamePrefix, UnitNamePrefix, Domain) \
    MACHINE_PARTS(Core) { \
        using AggNamePrefix##Group = TAggregatorGroup<UnitNamePrefix##Family, Domain>; \
        using AggNamePrefix##Family = AggNamePrefix##Group::TAggregatorFamily; \
        template <typename AggregatorType> \
        using AggNamePrefix##Stub = AggNamePrefix##Group::TAggregatorStub<AggregatorType>; \
        \
        UNIT_FAMILY_REGISTER( \
            Core, \
            AggNamePrefix ## Family, \
            TAggregatorFamilyInfo); \
    }

#define UNIT_EXPANSION_SINGLE_TRACKER(UnitNamePrefix, Expansion, Mode) \
    MACHINE_PARTS(Core) { \
        using UnitNamePrefix##Group = TBasicExpansionSingleTrackerGroup<Expansion>; \
        using UnitNamePrefix##Family = UnitNamePrefix##Group::TBasicExpansionSingleTrackerFamily; \
        template <typename TrackerType> \
        using UnitNamePrefix##Stub = UnitNamePrefix##Group::TBasicExpansionSingleTrackerStub<TrackerType, Mode>; \
        \
        UNIT_FAMILY_REGISTER( \
            Core, \
            UnitNamePrefix ## Family, \
            TBasicExpansionSingleTrackerFamilyInfo, Expansion, Mode); \
    }

#define UNIT_BAG_OF_WORDS_TRACKER(UnitNamePrefix, Expansion, Mode) \
    MACHINE_PARTS(Core) { \
        using UnitNamePrefix##Group = TBagOfWordsTrackerGroup<Expansion>; \
        using UnitNamePrefix##Family = UnitNamePrefix##Group::TBagOfWordsTrackerFamily; \
        template <typename Tracker> \
        using UnitNamePrefix##Stub = UnitNamePrefix##Group::TBagOfWordsTrackerStub<Tracker, Mode>; \
        \
        UNIT_FAMILY_REGISTER( \
            Core, \
            UnitNamePrefix ## Family, \
            TBagOfWordsTrackerFamilyInfo, Expansion, Mode); \
    }

namespace NTextMachine {
namespace NCore {
    // Trackers for known expansion types
    //
    UNIT_EXPANSION_SINGLE_TRACKER(TOriginalRequestSingleTracker, TExpansion::OriginalRequest, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_SINGLE_TRACKER(TRequestWithRegionNameSingleTracker, TExpansion::RequestWithRegionName, TQueryMode::UseOriginal)
    UNIT_EXPANSION_SINGLE_TRACKER(TRequestWithoutVerbsSingleTracker, TExpansion::RequestWithoutVerbs, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_SINGLE_TRACKER(TOriginalRequestWordsFilteredByDssmSSHardSingleTracker, TExpansion::OriginalRequestWordsFilteredByDssmSSHard, TQueryMode::DontUseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TOriginalRequestBagOfWordsTracker, TExpansion::OriginalRequest, TQueryMode::DontUseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TRequestMultitokensMultiTracker, TExpansion::RequestMultitokens, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TRequestMultitokensAggregatorAny, TRequestMultitokensMultiTracker, TAnyQuery)

    UNIT_EXPANSION_MULTI_TRACKER(TXfDtShowMultiTracker, TExpansion::XfDtShow, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TXfDtShowAggregatorAny, TXfDtShowMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TXfDtShowBagOfWordsTracker, TExpansion::XfDtShow, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TXfDtShowKnnMultiTracker, TExpansion::XfDtShowKnn, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TXfDtShowKnnAggregatorAny, TXfDtShowKnnMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TXfDtShowKnnBagOfWordsTracker, TExpansion::XfDtShowKnn, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TQueryToTextByXfDtShowKnnMultiTracker, TExpansion::QueryToTextByXfDtShowKnn, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TQueryToTextByXfDtShowKnnAggregatorAny, TQueryToTextByXfDtShowKnnMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TQueryToTextByXfDtShowKnnBagOfWordsTracker, TExpansion::QueryToTextByXfDtShowKnn, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TQueryToDocMultiTracker, TExpansion::QueryToDoc, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TQueryToDocAggregatorAny, TQueryToDocMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TQueryToDocBagOfWordsTracker, TExpansion::QueryToDoc, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TQfufMultiTracker, TExpansion::Qfuf, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TQfufAggregatorAny, TQfufMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TQfufBagOfWordsTracker, TExpansion::Qfuf, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TQueryToTextMultiTracker, TExpansion::QueryToText, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TQueryToTextAggregatorAny, TQueryToTextMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TQueryToTextBagOfWordsTracker, TExpansion::QueryToText, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TXfImgClicksMultiTracker, TExpansion::XfImgClicks, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TXfImgClicksAggregatorAny, TXfImgClicksMultiTracker, TAnyQuery)

    UNIT_EXPANSION_MULTI_TRACKER(TQfufFilteredByXfOneSeMultiTracker, TExpansion::QfufFilteredByXfOneSe, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TQfufFilteredByXfOneSeAggregatorAny, TQfufFilteredByXfOneSeMultiTracker, TAnyQuery)

    UNIT_EXPANSION_MULTI_TRACKER(TXfOneSeKnnMultiTracker, TExpansion::XfOneSeKnn, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TXfOneSeKnnAggregatorAny, TXfOneSeKnnMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TXfOneSeKnnBagOfWordsTracker, TExpansion::XfOneSeKnn, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TQueryToTextByXfOneSeKnnMultiTracker, TExpansion::QueryToTextByXfOneSeKnn, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TQueryToTextByXfOneSeKnnAggregatorAny, TQueryToTextByXfOneSeKnnMultiTracker, TAnyQuery)
    UNIT_BAG_OF_WORDS_TRACKER(TQueryToTextByXfOneSeKnnBagOfWordsTracker, TExpansion::QueryToTextByXfOneSeKnn, TQueryMode::UseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TVideoClickSimMultiTracker, TExpansion::VideoClickSim, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TVideoClickSimAggregatorAny, TVideoClickSimMultiTracker, TAnyQuery)

    UNIT_EXPANSION_MULTI_TRACKER(TKinopoiskSuggestMultiTracker, TExpansion::KinopoiskSuggest, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TKinopoiskSuggestAggregatorAny, TKinopoiskSuggestMultiTracker, TAnyQuery)
    // UNIT_BAG_OF_WORDS_TRACKER(TKinopoiskSuggestBagOfWordsTracker, TExpansion::KinopoiskSuggest, TQueryMode::UseOriginal)

    UNIT_EXPANSION_SINGLE_TRACKER(TGeoOriginalRequestSingleTracker, TExpansion::GeoOriginalRequest, TQueryMode::DontUseOriginal)

#define DECLARE_ADV_MACHINE_SELECT_TYPE(N)                                                                                                   \
    UNIT_EXPANSION_MULTI_TRACKER(TAdvMachineSelectType ## N ## MultiTracker, TExpansion::AdvMachineSelectType ## N, TQueryMode::DontUseOriginal) \
    UNIT_EXPANSION_AGGREGATOR(TAdvMachineSelectType ## N ## AggregatorAny, TAdvMachineSelectType ## N ## MultiTracker, TAnyQuery)

    DECLARE_ADV_MACHINE_SELECT_TYPE(123)
    DECLARE_ADV_MACHINE_SELECT_TYPE(26)
    DECLARE_ADV_MACHINE_SELECT_TYPE(98)
    DECLARE_ADV_MACHINE_SELECT_TYPE(57)
    DECLARE_ADV_MACHINE_SELECT_TYPE(101)
    DECLARE_ADV_MACHINE_SELECT_TYPE(89)
    DECLARE_ADV_MACHINE_SELECT_TYPE(96)
    DECLARE_ADV_MACHINE_SELECT_TYPE(Other)

    UNIT_EXPANSION_SINGLE_TRACKER(TContextMachinePageTitleSingleTracker, TExpansion::ContextMachinePageTitle, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_SINGLE_TRACKER(TContextMachineUrlSingleTracker, TExpansion::ContextMachineUrl, TQueryMode::DontUseOriginal)

#undef DECLARE_ADV_MACHINE_SELECT_TYPE

    UNIT_BAG_OF_WORDS_TRACKER(TXfImgClicksBagOfWordsTracker, TExpansion::XfImgClicks, TQueryMode::UseOriginal)

    // Experimental trackers
    //
    UNIT_EXPANSION_MULTI_TRACKER(TExperimentMultiTracker, TExpansion::Experiment, TQueryMode::UseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TExperimentAggregatorAny, TExperimentMultiTracker, TAnyQuery)

    UNIT_EXPANSION_MULTI_TRACKER(TExperiment0MultiTracker, TExpansion::Experiment0, TQueryMode::UseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment1MultiTracker, TExpansion::Experiment1, TQueryMode::UseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment2MultiTracker, TExpansion::Experiment2, TQueryMode::UseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment3MultiTracker, TExpansion::Experiment3, TQueryMode::UseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment4MultiTracker, TExpansion::Experiment4, TQueryMode::UseOriginal)

    UNIT_EXPANSION_AGGREGATOR(TExperiment0AggregatorAny, TExperiment0MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment1AggregatorAny, TExperiment1MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment2AggregatorAny, TExperiment2MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment3AggregatorAny, TExperiment3MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment4AggregatorAny, TExperiment4MultiTracker, TAnyQuery)

    UNIT_EXPANSION_MULTI_TRACKER(TExperiment5MultiTracker, TExpansion::Experiment5, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment6MultiTracker, TExpansion::Experiment6, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment7MultiTracker, TExpansion::Experiment7, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment8MultiTracker, TExpansion::Experiment8, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_MULTI_TRACKER(TExperiment9MultiTracker, TExpansion::Experiment9, TQueryMode::DontUseOriginal)

    UNIT_EXPANSION_AGGREGATOR(TExperiment5AggregatorAny, TExperiment5MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment6AggregatorAny, TExperiment6MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment7AggregatorAny, TExperiment7MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment8AggregatorAny, TExperiment8MultiTracker, TAnyQuery)
    UNIT_EXPANSION_AGGREGATOR(TExperiment9AggregatorAny, TExperiment9MultiTracker, TAnyQuery)

    UNIT_EXPANSION_SINGLE_TRACKER(TTelFullAttributeSingleTracker, TExpansion::TelFullAttribute, TQueryMode::DontUseOriginal)

    UNIT_EXPANSION_SINGLE_TRACKER(TFioFromOriginalRequestSingleTracker, TExpansion::FioFromOriginalRequest, TQueryMode::DontUseOriginal)

    UNIT_EXPANSION_MULTI_TRACKER(TAllFioFromOriginalRequestMultiTracker, TExpansion::AllFioFromOriginalRequest, TQueryMode::DontUseOriginal)
    UNIT_EXPANSION_AGGREGATOR(TAllFioFromOriginalRequestAggregatorAny, TAllFioFromOriginalRequestMultiTracker, TAnyQuery)

    UNIT_BAG_OF_WORDS_TRACKER(TExperimentBagOfWordsTracker, TExpansion::Experiment, TQueryMode::UseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment0BagOfWordsTracker, TExpansion::Experiment0, TQueryMode::UseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment1BagOfWordsTracker, TExpansion::Experiment1, TQueryMode::UseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment2BagOfWordsTracker, TExpansion::Experiment2, TQueryMode::UseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment3BagOfWordsTracker, TExpansion::Experiment3, TQueryMode::UseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment4BagOfWordsTracker, TExpansion::Experiment4, TQueryMode::UseOriginal)

    UNIT_BAG_OF_WORDS_TRACKER(TExperiment5BagOfWordsTracker, TExpansion::Experiment5, TQueryMode::DontUseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment6BagOfWordsTracker, TExpansion::Experiment6, TQueryMode::DontUseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment7BagOfWordsTracker, TExpansion::Experiment7, TQueryMode::DontUseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment8BagOfWordsTracker, TExpansion::Experiment8, TQueryMode::DontUseOriginal)
    UNIT_BAG_OF_WORDS_TRACKER(TExperiment9BagOfWordsTracker, TExpansion::Experiment9, TQueryMode::DontUseOriginal)

} // NCore
} // NTextMachine

#undef UNIT_BAG_OF_WORDS_TRACKER
#undef UNIT_EXPANSION_TRACKER
