#define UNIT_POSITIONLESS_STREAM_LIST(UnitNamePrefix, ...) \
    MACHINE_PARTS(Tracker) { \
        using UnitNamePrefix##Group = TPositionlessGroup<THitWeight::V0, __VA_ARGS__>; \
        using UnitNamePrefix##Family = UnitNamePrefix##Group::TPositionlessFamily; \
        template <typename Proxy> \
        using UnitNamePrefix##Stub = UnitNamePrefix##Group::TPositionlessStub<Proxy>; \
        \
        UNIT_FAMILY_REGISTER( \
            Tracker, \
            UnitNamePrefix ## Family, \
            TPositionlessFamilyInfo, \
            std::initializer_list<int>{__VA_ARGS__}); \
    } \
    MACHINE_PARTS(Tracker) { \
        using UnitNamePrefix##AccGroup = TPositionlessAccGroup<THitWeight::V0, __VA_ARGS__>; \
        using UnitNamePrefix##AccFamily = UnitNamePrefix##AccGroup::TPositionlessAccFamily; \
        template <typename Accumulator> \
        using UnitNamePrefix##AccStub = UnitNamePrefix##AccGroup::TPositionlessAccStub<Accumulator>; \
        \
        UNIT_FAMILY_REGISTER( \
            Tracker, \
            UnitNamePrefix ## AccFamily, \
            TPositionlessAccFamilyInfo, \
            std::initializer_list<int>{__VA_ARGS__}); \
    }

#define UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(UnitNamePrefix, HitWeight, ...) \
    MACHINE_PARTS(Tracker) { \
        using UnitNamePrefix##Group = TPositionlessGroup<HitWeight, __VA_ARGS__>; \
        using UnitNamePrefix##Family = UnitNamePrefix##Group::TPositionlessFamily; \
        template <typename Proxy> \
        using UnitNamePrefix##Stub = UnitNamePrefix##Group::TPositionlessStub<Proxy>; \
        \
        UNIT_FAMILY_REGISTER( \
            Tracker, \
            UnitNamePrefix ## Family, \
            TPositionlessFamilyInfo, \
            std::initializer_list<int>{__VA_ARGS__}); \
    } \
    MACHINE_PARTS(Tracker) { \
        using UnitNamePrefix##AccGroup = TPositionlessAccGroup<HitWeight, __VA_ARGS__>; \
        using UnitNamePrefix##AccFamily = UnitNamePrefix##AccGroup::TPositionlessAccFamily; \
        template <typename Accumulator> \
        using UnitNamePrefix##AccStub = UnitNamePrefix##AccGroup::TPositionlessAccStub<Accumulator>; \
        \
        UNIT_FAMILY_REGISTER( \
            Tracker, \
            UnitNamePrefix ## AccFamily, \
            TPositionlessAccFamilyInfo, \
            std::initializer_list<int>{__VA_ARGS__}); \
    }

namespace NTextMachine {
namespace NCore {
    UNIT_POSITIONLESS_STREAM_LIST(TUrlPositionless, TStream::Url)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TUrlAttenPositionless, THitWeight::AttenV1, TStream::Url)

    UNIT_POSITIONLESS_STREAM_LIST(TTitlePositionless, TStream::Title)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TTitleAttenPositionless, THitWeight::AttenV1, TStream::Title)
    UNIT_POSITIONLESS_STREAM_LIST(TBodyPositionless, TStream::Body)
    UNIT_POSITIONLESS_STREAM_LIST(TFullTextPositionless, TStream::FullText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TFullTextOldTRAttenPositionless, THitWeight::OldTRAtten, TStream::FullText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TFullTextHiRelAttenPositionless, THitWeight::TxtHiRel, TStream::FullText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TFullTextHeadAttenPositionless, THitWeight::TxtHead, TStream::FullText)
    UNIT_POSITIONLESS_STREAM_LIST(TTextPositionless, TStream::Title, TStream::Body)
    UNIT_POSITIONLESS_STREAM_LIST(TLinksPositionless, TStream::LinksInternal,
        TStream::LinksExternalDpr, TStream::CatalogueYandex, TStream::CatalogueDmoz)
    UNIT_POSITIONLESS_STREAM_LIST(TCataloguePositionless, TStream::CatalogueDmoz,
        TStream::CatalogueYandex)
    UNIT_POSITIONLESS_STREAM_LIST(TInternalLinksPositionless, TStream::LinksInternal)
    UNIT_POSITIONLESS_STREAM_LIST(TExternalLinksPositionless, TStream::LinksExternal)
    UNIT_POSITIONLESS_STREAM_LIST(TAllLinksPositionless, TStream::LinksExternal, TStream::LinksInternal)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TDprLinksPositionless, THitWeight::V1, TStream::LinksExternalDpr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TTicLinksPositionless, THitWeight::V1, TStream::LinksExternalTic)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TCorrectedCtrPositionless, THitWeight::V1, TStream::CorrectedCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TCorrectedCtrV2Positionless, THitWeight::V2, TStream::CorrectedCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TCorrectedCtrV4Positionless, THitWeight::V4, TStream::CorrectedCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TCorrectedCtrLongPeriodPositionless, THitWeight::V1, TStream::CorrectedCtrLongPeriod)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLongClickPositionless, THitWeight::V1, TStream::LongClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLongClickV2Positionless, THitWeight::V2, TStream::LongClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLongClickV4Positionless, THitWeight::V4, TStream::LongClick)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOneClickPositionless, THitWeight::V1, TStream::OneClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOneClickAttenPositionless, THitWeight::AttenV1, TStream::OneClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOneClickV2Positionless, THitWeight::V2, TStream::OneClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOneClickV4Positionless, THitWeight::V4, TStream::OneClick)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBrowserPageRankPositionless, THitWeight::V1, TStream::BrowserPageRank)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBrowserPageRankV2Positionless, THitWeight::V2, TStream::BrowserPageRank)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBrowserPageRankV4Positionless, THitWeight::V4, TStream::BrowserPageRank)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBrowserPageRankAttenPositionless, THitWeight::AttenV1, TStream::BrowserPageRank)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAvgDTWeightedByRankMobilePositionless, THitWeight::V1, TStream::AvgDTWeightedByRankMobile)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAvgDTWeightedByRankMobileAttenPositionless, THitWeight::AttenV1, TStream::AvgDTWeightedByRankMobile)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSplitDwellTimePositionless, THitWeight::V1, TStream::SplitDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSplitDwellTimeAttenPositionless, THitWeight::AttenV1, TStream::SplitDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSimpleClickPositionless, THitWeight::V1, TStream::SimpleClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSimpleClickAttenPositionless, THitWeight::AttenV1, TStream::SimpleClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSamplePeriodDayFrcPositionless, THitWeight::V1, TStream::SamplePeriodDayFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYabarVisitsPositionless, THitWeight::V1, TStream::YabarVisits)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYabarTimePositionless, THitWeight::V1, TStream::YabarTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYabarTimeAttenPositionless, THitWeight::AttenV1, TStream::YabarTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLongClickSPPositionless, THitWeight::V1, TStream::LongClickSP)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLongClickSPAttenPositionless, THitWeight::AttenV1, TStream::LongClickSP)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOneClickFrcXfSpPositionless, THitWeight::V1, TStream::OneClickFrcXfSp)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBQPRSamplePositionless, THitWeight::V1, TStream::BQPRSample)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBQPRSampleAttenPositionless, THitWeight::AttenV1, TStream::BQPRSample)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSamplePeriodDayFrcAttenPositionless, THitWeight::AttenV1, TStream::SamplePeriodDayFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TCorrectedCtrXFactorPositionless, THitWeight::V1, TStream::CorrectedCtrXFactor)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TQueryDwellTimePositionless, THitWeight::V1, TStream::QueryDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TQueryDwellTimeAttenPositionless, THitWeight::AttenV1, TStream::QueryDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TRandomLogDBM35Positionless, THitWeight::V1, TStream::RandomLogDBM35)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOnotoleAttenPositionless, THitWeight::AttenV1, TStream::Onotole)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOnotoleV2Positionless, THitWeight::V2, TStream::Onotole)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOnotoleV4Positionless, THitWeight::V4, TStream::Onotole)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOnotolePositionless, THitWeight::V1, TStream::Onotole)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TNHopSumDwellTimePositionless, THitWeight::V1, TStream::NHopSumDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TNHopIsFinalPositionless, THitWeight::V1, TStream::NHopIsFinal)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TNHopIsFinalAttenPositionless, THitWeight::AttenV1, TStream::NHopIsFinal)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TFirstClickDtXfPositionless, THitWeight::V1, TStream::FirstClickDtXf)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TFirstClickDtXfAttenPositionless, THitWeight::AttenV1, TStream::FirstClickDtXf)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLinkAnnIndicatorPositionless, THitWeight::V1, TStream::LinkAnnIndicator)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLinkAnnFloatMultiplicityPositionless, THitWeight::V1, TStream::LinkAnnFloatMultiplicity)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLinkAnnFloatMultiplicityAttenPositionless, THitWeight::AttenV1, TStream::LinkAnnFloatMultiplicity)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoQusmPositionless, THitWeight::V1, TStream::VideoQusm)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoMaxClicksPercentPositionless, THitWeight::V1, TStream::VideoMaxClicksPercent)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoPctrNewPositionless, THitWeight::V1, TStream::VideoPctrNew)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoNHopPositionless, THitWeight::V1, TStream::VideoNHop)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoCorrectedClicksFrcPositionless, THitWeight::V1, TStream::VideoCorrectedClicksFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoCorrectedClicksFrcAttenV1Positionless, THitWeight::AttenV1, TStream::VideoCorrectedClicksFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoCorrectedClicksUFrcPositionless, THitWeight::V1, TStream::VideoCorrectedClicksUFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoSpeechToTextPositionless, THitWeight::V1, TStream::VideoSpeechToText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVideoOcrPositionless, THitWeight::V1, TStream::VideoOcr)
    UNIT_POSITIONLESS_STREAM_LIST(TVideoUnitedFactorAnnPositionless, TStream::VideoUnitedFactorAnn)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesGoXFRCPositionless, THitWeight::V1, TStream::ImagesGoXFRC)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesOCRPositionless, THitWeight::V1, TStream::ImagesOCR)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesOCRv2Positionless, THitWeight::V1, TStream::ImagesOCRv2)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesRightXFRCPositionless, THitWeight::V1, TStream::ImagesRightXFRC)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesCVTagsPositionless, THitWeight::V1, TStream::ImagesCVTags)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesGoClicksPositionless, THitWeight::V1, TStream::ImagesGoClicks)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesYaClicksPositionless, THitWeight::V1, TStream::ImagesYaClicks)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesTolokaOkPositionless, THitWeight::V1, TStream::ImagesTolokaOk)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesTolokaBadPositionless, THitWeight::V1, TStream::ImagesTolokaBad)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TImagesTitlePositionless, THitWeight::V1, TStream::ImagesTitle)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetaPolyGen8Positionless, THitWeight::V1, TStream::MetaPolyGen8)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TLongClickMobilePositionless, THitWeight::V1, TStream::LongClickMobile)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerCommQueriesCtrPositionless, THitWeight::V1, TStream::AdvBannerCommQueriesCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerCommQueriesXfCtrPositionless, THitWeight::V1, TStream::AdvBannerCommQueriesXfCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerPhraseRsyaCtrAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerPhraseRsyaCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerPhraseRsyaFrcAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerPhraseRsyaFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerPhraseRsyaFrcPositionless, THitWeight::V1, TStream::AdvBannerPhraseRsyaFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerPhraseRsyaHistoryCpcPositionless, THitWeight::V1, TStream::AdvBannerPhraseRsyaHistoryCpc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerPhraseRsyaHistoryCpcAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerPhraseRsyaHistoryCpc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerRsyaQueryCtrAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerRsyaQueryCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerRsyaQueryCtrPositionless, THitWeight::V1, TStream::AdvBannerRsyaQueryCtr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerRsyaQueryFrcPositionless, THitWeight::V1, TStream::AdvBannerRsyaQueryFrc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerSpyLogTitlePositionless, THitWeight::V1, TStream::AdvBannerSpyLogTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerTextPositionless, THitWeight::V1, TStream::AdvBannerText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerTextAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerTitleAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerTitlePositionless, THitWeight::V1, TStream::AdvBannerTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvLandingPageTitlePositionless, THitWeight::V1, TStream::AdvLandingPageTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBQPRPositionless, THitWeight::V1, TStream::BQPR)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBQPRAttenPositionless, THitWeight::AttenV1, TStream::BQPR)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvLandingPageTextPositionless, THitWeight::V1, TStream::AdvLandingPageText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvLandingPageTextAttenPositionless, THitWeight::AttenV1, TStream::AdvLandingPageText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerCommQueriesBidAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerCommQueriesBid)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerPhraseHistoryCpcPositionless, THitWeight::V1, TStream::AdvBannerPhraseHistoryCpc)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerRsyaQueryBidPositionless, THitWeight::V1, TStream::AdvBannerRsyaQueryBid)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvLandingPageTitleAttenPositionless, THitWeight::AttenV1, TStream::AdvLandingPageTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerQueryCostFrPositionless, THitWeight::V1, TStream::AdvBannerQueryCostFr)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TUserQueryPositionless, THitWeight::V1, TStream::UserQuery)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerOriginalPhrasePositionless, THitWeight::V1, TStream::AdvBannerOriginalPhrase)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerOriginalPhraseBidPositionless, THitWeight::V1, TStream::AdvBannerOriginalPhraseBid)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerOriginalPhraseBidAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerOriginalPhraseBid)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerOriginalPhraseBidToMaxBidPositionless, THitWeight::V1, TStream::AdvBannerOriginalPhraseBidToMaxBid)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAdvBannerOriginalPhraseBidToMaxBidAttenPositionless, THitWeight::AttenV1, TStream::AdvBannerOriginalPhraseBidToMaxBid)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMapsOrgAddressPositionless, THitWeight::V1, TStream::MapsOrgAddress)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMapsOrgAddressAttenPositionless, THitWeight::AttenV1, TStream::MapsOrgAddress)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMapsOrgNamePositionless, THitWeight::V1, TStream::MapsOrgName)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSerpMapsClickSOFRCAttenPositionless, THitWeight::AttenV1, TStream::SerpMapsClickSOFRC)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TSerpMapsClickSQFRCAttenPositionless, THitWeight::AttenV1, TStream::SerpMapsClickSQFRC)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TOrgSummaryPositionless, THitWeight::V1, TStream::OrgSummary)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketModelAliasPositionless, THitWeight::V1, TStream::MarketModelAlias)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketModelAliasAttenPositionless, THitWeight::AttenV1, TStream::MarketModelAlias)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageLinkDataPositionless, THitWeight::V1, TStream::MarketImageLinkData)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageLinkDataAttenPositionless, THitWeight::AttenV1, TStream::MarketImageLinkData)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageAltDataPositionless, THitWeight::V1, TStream::MarketImageAltData)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageAltDataAttenPositionless, THitWeight::AttenV1, TStream::MarketImageAltData)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageShowsTimePositionless, THitWeight::V1, TStream::MarketImageShowsTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageShowsTimeAttenPositionless, THitWeight::AttenV1, TStream::MarketImageShowsTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageQueryShowsTimePositionless, THitWeight::V1, TStream::MarketImageQueryShowsTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageQueryShowsTimeAttenPositionless, THitWeight::AttenV1, TStream::MarketImageQueryShowsTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageDwellTimePositionless, THitWeight::V1, TStream::MarketImageDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageDwellTimeAttenPositionless, THitWeight::AttenV1, TStream::MarketImageDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageQueryDwellTimePositionless, THitWeight::V1, TStream::MarketImageQueryDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageQueryDwellTimeAttenPositionless, THitWeight::AttenV1, TStream::MarketImageQueryDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageDocDwellTimePositionless, THitWeight::V1, TStream::MarketImageDocDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageDocDwellTimeAttenPositionless, THitWeight::AttenV1, TStream::MarketImageDocDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageShowsPositionless, THitWeight::V1, TStream::MarketImageShows)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageShowsAttenPositionless, THitWeight::AttenV1, TStream::MarketImageShows)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageClicksPositionless, THitWeight::V1, TStream::MarketImageClicks)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageClicksAttenPositionless, THitWeight::AttenV1, TStream::MarketImageClicks)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageQueryClicksPositionless, THitWeight::V1, TStream::MarketImageQueryClicks)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketImageQueryClicksAttenPositionless, THitWeight::AttenV1, TStream::MarketImageQueryClicks)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketDescriptionPositionless, THitWeight::V1, TStream::MarketDescription)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketDescriptionAttenPositionless, THitWeight::AttenV1, TStream::MarketDescription)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketWasOrderedPositionless, THitWeight::V1, TStream::MarketWasOrdered)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketWasOrderedAttenPositionless, THitWeight::AttenV1, TStream::MarketWasOrdered)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_5000_9600Positionless, THitWeight::V1, TStream::MarketKNN_5000_9600)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_5000_9600AttenPositionless, THitWeight::AttenV1, TStream::MarketKNN_5000_9600)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_9600_10000Positionless, THitWeight::V1, TStream::MarketKNN_9600_10000)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_9600_10000AttenPositionless, THitWeight::AttenV1, TStream::MarketKNN_9600_10000)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_10000_11000Positionless, THitWeight::V1, TStream::MarketKNN_10000_11000)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_10000_11000AttenPositionless, THitWeight::AttenV1, TStream::MarketKNN_10000_11000)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_11000_20000Positionless, THitWeight::V1, TStream::MarketKNN_11000_20000)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketKNN_11000_20000AttenPositionless, THitWeight::AttenV1, TStream::MarketKNN_11000_20000)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketManufacturerPositionless, THitWeight::V1, TStream::YaMarketManufacturer)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketManufacturerAttenPositionless, THitWeight::AttenV1, TStream::YaMarketManufacturer)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketCategoryNamePositionless, THitWeight::V1, TStream::YaMarketCategoryName)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketCategoryNameAttenPositionless, THitWeight::AttenV1, TStream::YaMarketCategoryName)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketAliasesPositionless, THitWeight::V1, TStream::YaMarketAliases)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketAliasesAttenPositionless, THitWeight::AttenV1, TStream::YaMarketAliases)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketUrlPositionless, THitWeight::V1, TStream::YaMarketUrl)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketUrlAttenPositionless, THitWeight::AttenV1, TStream::YaMarketUrl)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferDescriptionPositionless, THitWeight::V1, TStream::YaMarketOfferDescription)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferDescriptionAttenPositionless, THitWeight::AttenV1, TStream::YaMarketOfferDescription)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferSalesNotesPositionless, THitWeight::V1, TStream::YaMarketOfferSalesNotes)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferSalesNotesAttenPositionless, THitWeight::AttenV1, TStream::YaMarketOfferSalesNotes)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferTitlePositionless, THitWeight::V1, TStream::YaMarketOfferTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferTitleAttenPositionless, THitWeight::AttenV1, TStream::YaMarketOfferTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferUrlPositionless, THitWeight::V1, TStream::YaMarketOfferUrl)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOfferUrlAttenPositionless, THitWeight::AttenV1, TStream::YaMarketOfferUrl)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketSplitDwellTimePositionless, THitWeight::V1, TStream::YaMarketSplitDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketSplitDwellTimeAttenPositionless, THitWeight::AttenV1, TStream::YaMarketSplitDwellTime)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOneClickPositionless, THitWeight::V1, TStream::YaMarketOneClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketOneClickAttenPositionless, THitWeight::AttenV1, TStream::YaMarketOneClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketLongClickPositionless, THitWeight::V1, TStream::YaMarketLongClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketLongClickAttenPositionless, THitWeight::AttenV1, TStream::YaMarketLongClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketLongClick120SqrtPositionless, THitWeight::V1, TStream::YaMarketLongClick120Sqrt)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketLongClick120SqrtAttenPositionless, THitWeight::AttenV1, TStream::YaMarketLongClick120Sqrt)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketSimpleClickPositionless, THitWeight::V1, TStream::YaMarketSimpleClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketSimpleClickAttenPositionless, THitWeight::AttenV1, TStream::YaMarketSimpleClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketFirstLastClickMobilePositionless, THitWeight::V1, TStream::YaMarketFirstLastClickMobile)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TYaMarketFirstLastClickMobileAttenPositionless, THitWeight::AttenV1, TStream::YaMarketFirstLastClickMobile)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueAliasOfWhiteModelPositionless, THitWeight::V1, TStream::MarketBlueAliasOfWhiteModel)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueAliasOfWhiteModelAttenPositionless, THitWeight::AttenV1, TStream::MarketBlueAliasOfWhiteModel)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueTitleOfWhiteModelPositionless, THitWeight::V1, TStream::MarketBlueTitleOfWhiteModel)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueTitleOfWhiteModelAttenPositionless, THitWeight::AttenV1, TStream::MarketBlueTitleOfWhiteModel)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueDescriptionOfWhiteOfferPositionless, THitWeight::V1, TStream::MarketBlueDescriptionOfWhiteOffer)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueDescriptionOfWhiteOfferAttenPositionless, THitWeight::AttenV1, TStream::MarketBlueDescriptionOfWhiteOffer)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueTitleOfWhiteOfferPositionless, THitWeight::V1, TStream::MarketBlueTitleOfWhiteOffer)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketBlueTitleOfWhiteOfferAttenPositionless, THitWeight::AttenV1, TStream::MarketBlueTitleOfWhiteOffer)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketTitlePositionless, THitWeight::V1, TStream::MarketTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketTitleAttenPositionless, THitWeight::AttenV1, TStream::MarketTitle)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketingDescriptionPositionless, THitWeight::V1, TStream::MarketingDescription)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketingDescriptionAttenPositionless, THitWeight::AttenV1, TStream::MarketingDescription)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBlueMarketingDescriptionOfModelPositionless, THitWeight::V1, TStream::BlueMarketingDescriptionOfModel)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBlueMarketingDescriptionOfModelAttenPositionless, THitWeight::AttenV1, TStream::BlueMarketingDescriptionOfModel)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBlueMicroDescriptionOfModelPositionless, THitWeight::V1, TStream::BlueMicroDescriptionOfModel)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TBlueMicroDescriptionOfModelAttenPositionless, THitWeight::AttenV1, TStream::BlueMicroDescriptionOfModel)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAliceMusicAlbumTitleAttenPositionless, THitWeight::AttenV1, TStream::AliceMusicAlbumTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAliceMusicArtistNameAttenPositionless, THitWeight::AttenV1, TStream::AliceMusicArtistName)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TAliceMusicArtistNamePositionless, THitWeight::V1, TStream::AliceMusicArtistName)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMarketCPAQueryPositionless, THitWeight::V1, TStream::MarketCPAQuery)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMskuOfferSearchTextPositionless, THitWeight::V1, TStream::MskuOfferSearchText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMskuOfferSearchTextAttenPositionless, THitWeight::AttenV1, TStream::MskuOfferSearchText)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMskuOfferTitlePositionless, THitWeight::V1, TStream::MskuOfferTitle)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMskuOfferTitleAttenPositionless, THitWeight::AttenV1, TStream::MskuOfferTitle)


    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocBQRPPositionless, THitWeight::V1, TStream::MetadocBQRP)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocBQRPAttenPositionless, THitWeight::AttenV1, TStream::MetadocBQRP)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocFirstClickDTXFPositionless, THitWeight::V1, TStream::MetadocFirstClickDTXF)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocFirstClickDTXFAttenPositionless, THitWeight::AttenV1, TStream::MetadocFirstClickDTXF)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocLongClickPositionless, THitWeight::V1, TStream::MetadocLongClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocLongClickAttenPositionless, THitWeight::AttenV1, TStream::MetadocLongClick)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocLongClickSPPositionless, THitWeight::V1, TStream::MetadocLongClickSP)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocLongClickSPAttenPositionless, THitWeight::AttenV1, TStream::MetadocLongClickSP)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocOneClickPositionless, THitWeight::V1, TStream::MetadocOneClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocOneClickAttenPositionless, THitWeight::AttenV1, TStream::MetadocOneClick)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocSimpleClickPositionless, THitWeight::V1, TStream::MetadocSimpleClick)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocSimpleClickAttenPositionless, THitWeight::AttenV1, TStream::MetadocSimpleClick)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocSplitDTPositionless, THitWeight::V1, TStream::MetadocSplitDT)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocSplitDTAttenPositionless, THitWeight::AttenV1, TStream::MetadocSplitDT)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocBQPRSamplePositionless, THitWeight::V1, TStream::MetadocBQPRSample)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocBQPRSampleAttenPositionless, THitWeight::AttenV1, TStream::MetadocBQPRSample)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocNHOPPositionless, THitWeight::V1, TStream::MetadocNHOP)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TMetadocNHOPAttenPositionless, THitWeight::AttenV1, TStream::MetadocNHOP)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVendorNamePositionless, THitWeight::V1, TStream::VendorName)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TVendorNameAttenPositionless, THitWeight::AttenV1, TStream::VendorName)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TShopNamePositionless, THitWeight::V1, TStream::ShopName)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TShopNameAttenPositionless, THitWeight::AttenV1, TStream::ShopName)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TShopCategoriesPositionless, THitWeight::V1, TStream::ShopCategories)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TShopCategoriesAttenPositionless, THitWeight::AttenV1, TStream::ShopCategories)

    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TNidNamesPositionless, THitWeight::V1, TStream::NidNames)
    UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST(TNidNamesAttenPositionless, THitWeight::AttenV1, TStream::NidNames)

} // NCore
} // NTextMachine

#undef UNIT_POSITIONLESS_STREAM_LIST
#undef UNIT_POSITIONLESS_WEIGHTED_STREAM_LIST
