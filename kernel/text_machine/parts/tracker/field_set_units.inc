#define UNIT_BCLM_MIX_STREAM(AliasName, StreamName) \
    MACHINE_PARTS(Tracker) { \
        using AliasName = TBclmMixUnit<StreamName>;\
        UNIT_REGISTER(Tracker, AliasName, TBclmMixUnitInfo, StreamName); \
    }

#define UNIT_BCLM_MIX_STREAM_WEIGHTED(AliasName, StreamName, Weight) \
    MACHINE_PARTS(Tracker) { \
        using AliasName = TBclmMixUnit<StreamName, Weight>;\
        UNIT_REGISTER(Tracker, AliasName, TBclmMixUnitInfo, StreamName); \
    }


#define UNIT_BCLM_SIZE1_STREAM(AliasName, StreamName) \
    MACHINE_PARTS(Tracker) { \
        using AliasName = TBclmSize1Unit<StreamName>;\
        UNIT_REGISTER(Tracker, AliasName, TBclmSize1UnitInfo, StreamName); \
    }

#define UNIT_BCLM_SIZE1_CUSTOM_WEIGHT_STREAM(AliasName, StreamName, HitType) \
    MACHINE_PARTS(Tracker) { \
        using AliasName = TBclmSize1Unit<StreamName, HitType>;\
        UNIT_REGISTER(Tracker, AliasName, TBclmSize1UnitInfo, StreamName); \
    }

namespace NTextMachine {
namespace NCore {
    UNIT_BCLM_MIX_STREAM_WEIGHTED(TTitleBclmMixUnit, TStream::Title, THitWeight::V0);
    UNIT_BCLM_MIX_STREAM_WEIGHTED(TUrlBclmMixUnit, TStream::Url, THitWeight::V0);
    UNIT_BCLM_MIX_STREAM_WEIGHTED(TBodyBclmMixUnit, TStream::Body, THitWeight::V0);
    UNIT_BCLM_MIX_STREAM(TOneClickFrcXfSpBclmMixUnit, TStream::OneClickFrcXfSp);
    UNIT_BCLM_MIX_STREAM(TBQPRSampleBclmMixUnit, TStream::BQPRSample);
    UNIT_BCLM_MIX_STREAM(TOnotoleBclmMixUnit, TStream::Onotole);
    UNIT_BCLM_MIX_STREAM(TNHopSumDwellTimeBclmMixUnit, TStream::NHopSumDwellTime);
    UNIT_BCLM_MIX_STREAM(TFirstClickDtXfBclmMixUnit, TStream::FirstClickDtXf);
    UNIT_BCLM_MIX_STREAM(TMetaPolyGen8BclmMixUnit, TStream::MetaPolyGen8);
    UNIT_BCLM_MIX_STREAM(TAdvBannerPhraseRsyaFrcBclmMixUnit, TStream::AdvBannerPhraseRsyaFrc);
    UNIT_BCLM_MIX_STREAM(TBQPRBclmMixUnit, TStream::BQPR);
    UNIT_BCLM_MIX_STREAM(TAdvBannerCommQueriesCtrBclmMixUnit, TStream::AdvBannerCommQueriesCtr);
    UNIT_BCLM_MIX_STREAM(TAdvBannerCommQueriesXfCtrBclmMixUnit, TStream::AdvBannerCommQueriesXfCtr);
    UNIT_BCLM_MIX_STREAM(TAdvBannerSpyLogTitleBclmMixUnit, TStream::AdvBannerSpyLogTitle);
    UNIT_BCLM_MIX_STREAM(TAdvBannerTitleBclmMixUnit, TStream::AdvBannerTitle);
    UNIT_BCLM_MIX_STREAM(TCorrectedCtrBclmMixUnit, TStream::CorrectedCtr);
    UNIT_BCLM_MIX_STREAM(TAdvLandingPageTextBclmMixUnit, TStream::AdvLandingPageText);
    UNIT_BCLM_MIX_STREAM(TAdvBannerPhraseRsyaHistoryCpcBclmMixUnit, TStream::AdvBannerPhraseRsyaHistoryCpc);
    UNIT_BCLM_MIX_STREAM(TAdvBannerRsyaQueryBidBclmMixUnit, TStream::AdvBannerRsyaQueryBid);
    UNIT_BCLM_MIX_STREAM(TAdvLandingPageTitleBclmMixUnit, TStream::AdvLandingPageTitle);
    UNIT_BCLM_MIX_STREAM(TAdvBannerOriginalPhraseBclmMixUnit, TStream::AdvBannerOriginalPhrase);
    UNIT_BCLM_MIX_STREAM(TAdvBannerOriginalPhraseBidBclmMixUnit, TStream::AdvBannerOriginalPhraseBid);
    UNIT_BCLM_MIX_STREAM(TImagesYaClicksBclmMixUnit, TStream::ImagesYaClicks);

    UNIT_BCLM_MIX_STREAM(TVideoQusmBclmMixUnit, TStream::VideoQusm);
    UNIT_BCLM_MIX_STREAM(TVideoSpeechToTextBclmMixUnit, TStream::VideoSpeechToText);

    UNIT_BCLM_MIX_STREAM(TOrgSummaryBclmMixUnit, TStream::OrgSummary);

    UNIT_BCLM_MIX_STREAM(TBrowserPageRankBclmMixUnit, TStream::BrowserPageRank);
    UNIT_BCLM_MIX_STREAM(TMarketModelAliasBclmMixUnit, TStream::MarketModelAlias);
    UNIT_BCLM_MIX_STREAM(TLongClickSPBclmMixUnit, TStream::LongClickSP);
    UNIT_BCLM_MIX_STREAM(TMarketBlueTitleOfWhiteOfferBclmMixUnit, TStream::MarketBlueTitleOfWhiteOffer);
    UNIT_BCLM_MIX_STREAM(TNHopIsFinalBclmMixUnit, TStream::NHopIsFinal);
    UNIT_BCLM_MIX_STREAM(TSimpleClickBclmMixUnit, TStream::SimpleClick);
    UNIT_BCLM_MIX_STREAM(TLongClickBclmMixUnit, TStream::LongClick);
    UNIT_BCLM_MIX_STREAM(TYaMarketOfferUrlBclmMixUnit, TStream::YaMarketOfferUrl);
    UNIT_BCLM_MIX_STREAM(TAvgDTWeightedByRankMobileBclmMixUnit, TStream::AvgDTWeightedByRankMobile);
    UNIT_BCLM_MIX_STREAM(TYaMarketOfferTitleBclmMixUnit, TStream::YaMarketOfferTitle);
    UNIT_BCLM_MIX_STREAM(TMarketBlueTitleOfWhiteModelBclmMixUnit, TStream::MarketBlueTitleOfWhiteModel);
    UNIT_BCLM_MIX_STREAM(TMarketTitleBclmMixUnit, TStream::MarketTitle);
    UNIT_BCLM_MIX_STREAM(TYaMarketAliasesBclmMixUnit, TStream::YaMarketAliases);
    UNIT_BCLM_MIX_STREAM(TYaMarketCategoryNameBclmMixUnit, TStream::YaMarketCategoryName);
    UNIT_BCLM_MIX_STREAM(TYaMarketManufacturerBclmMixUnit, TStream::YaMarketManufacturer);
    UNIT_BCLM_MIX_STREAM(TOneClickBclmMixUnit, TStream::OneClick);
    UNIT_BCLM_MIX_STREAM(TMarketBlueDescriptionOfWhiteOfferBclmMixUnit, TStream::MarketBlueDescriptionOfWhiteOffer);
    UNIT_BCLM_MIX_STREAM(TYaMarketUrlBclmMixUnit, TStream::YaMarketUrl);
    UNIT_BCLM_MIX_STREAM(TAliceMusicArtistNameBclmMixUnit, TStream::AliceMusicArtistName)
    UNIT_BCLM_MIX_STREAM(TMarketCPAQueryBclmMixUnit, TStream::MarketCPAQuery)

    UNIT_BCLM_SIZE1_STREAM(TSplitDwellTimeBclmSize1Unit, TStream::SplitDwellTime);
    UNIT_BCLM_SIZE1_STREAM(TNHopIsFinalBclmSize1Unit, TStream::NHopIsFinal);
    UNIT_BCLM_SIZE1_STREAM(TOneClickFrcXfSpBclmSize1Unit, TStream::OneClickFrcXfSp);
    UNIT_BCLM_SIZE1_STREAM(TOneClickBclmSize1Unit, TStream::OneClick);
    UNIT_BCLM_SIZE1_STREAM(TSimpleClickBclmSize1Unit, TStream::SimpleClick);
    UNIT_BCLM_SIZE1_STREAM(TBQPRSampleBclmSize1Unit, TStream::BQPRSample);
    UNIT_BCLM_SIZE1_STREAM(TLongClickSPBclmSize1Unit, TStream::LongClickSP);
    UNIT_BCLM_SIZE1_STREAM(TLongClickBclmSize1Unit, TStream::LongClick);
    UNIT_BCLM_SIZE1_STREAM(TSamplePeriodDayFrcBclmSize1Unit, TStream::SamplePeriodDayFrc);
    UNIT_BCLM_SIZE1_STREAM(TCorrectedCtrXFactorBclmSize1Unit, TStream::CorrectedCtrXFactor);
    UNIT_BCLM_SIZE1_STREAM(TQueryDwellTimeBclmSize1Unit, TStream::QueryDwellTime);
    UNIT_BCLM_SIZE1_STREAM(TRandomLogDBM35BclmSize1Unit, TStream::RandomLogDBM35);
    UNIT_BCLM_SIZE1_STREAM(TOnotoleBclmSize1Unit, TStream::Onotole);
    UNIT_BCLM_SIZE1_STREAM(TFirstClickDtXfBclmSize1Unit, TStream::FirstClickDtXf);
    UNIT_BCLM_SIZE1_STREAM(TAdvBannerSpyLogTitleBclmSize1Unit, TStream::AdvBannerSpyLogTitle);
    UNIT_BCLM_SIZE1_STREAM(TAdvBannerCommQueriesCtrBclmSize1Unit, TStream::AdvBannerCommQueriesCtr);
    UNIT_BCLM_SIZE1_STREAM(TCorrectedCtrBclmSize1Unit, TStream::CorrectedCtr);
    UNIT_BCLM_SIZE1_STREAM(TBQPRBclmSize1Unit, TStream::BQPR);
    UNIT_BCLM_SIZE1_STREAM(TAdvLandingPageTextBclmSize1Unit, TStream::AdvLandingPageText);
    UNIT_BCLM_SIZE1_STREAM(TAdvBannerQueryCostFrBclmSize1Unit, TStream::AdvBannerQueryCostFr);
    UNIT_BCLM_SIZE1_STREAM(TAdvBannerOriginalPhraseBidBclmSize1Unit, TStream::AdvBannerOriginalPhraseBid);
    UNIT_BCLM_SIZE1_STREAM(TBrowserPageRankBclmSize1Unit, TStream::BrowserPageRank);
    UNIT_BCLM_SIZE1_STREAM(TAvgDTWeightedByRankMobileBclmSize1Unit, TStream::AvgDTWeightedByRankMobile);
    UNIT_BCLM_SIZE1_CUSTOM_WEIGHT_STREAM(TFullTextBclmSize1Unit, TStream::FullText, THitWeight::V0);
    UNIT_BCLM_SIZE1_CUSTOM_WEIGHT_STREAM(TTitleBclmSize1Unit, TStream::Title, THitWeight::V0);
    UNIT_BCLM_SIZE1_CUSTOM_WEIGHT_STREAM(TBodyBclmSize1Unit, TStream::Body, THitWeight::V0);

    UNIT_BCLM_SIZE1_STREAM(TMarketModelAliasBclmSize1Unit, TStream::MarketModelAlias);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageLinkDataBclmSize1Unit, TStream::MarketImageLinkData);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageAltDataBclmSize1Unit, TStream::MarketImageAltData);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageShowsTimeBclmSize1Unit, TStream::MarketImageShowsTime);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageQueryShowsTimeBclmSize1Unit, TStream::MarketImageQueryShowsTime);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageDwellTimeBclmSize1Unit, TStream::MarketImageDwellTime);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageQueryDwellTimeBclmSize1Unit, TStream::MarketImageQueryDwellTime);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageDocDwellTimeBclmSize1Unit, TStream::MarketImageDocDwellTime);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageShowsBclmSize1Unit, TStream::MarketImageShows);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageClicksBclmSize1Unit, TStream::MarketImageClicks);
    UNIT_BCLM_SIZE1_STREAM(TMarketImageQueryClicksBclmSize1Unit, TStream::MarketImageQueryClicks);
    UNIT_BCLM_SIZE1_STREAM(TMarketDescriptionBclmSize1Unit, TStream::MarketDescription);
    UNIT_BCLM_SIZE1_STREAM(TMarketWasOrderedBclmSize1Unit, TStream::MarketWasOrdered);
    UNIT_BCLM_SIZE1_STREAM(TMarketKNN_5000_9600BclmSize1Unit, TStream::MarketKNN_5000_9600);
    UNIT_BCLM_SIZE1_STREAM(TMarketKNN_9600_10000BclmSize1Unit, TStream::MarketKNN_9600_10000);
    UNIT_BCLM_SIZE1_STREAM(TMarketKNN_10000_11000BclmSize1Unit, TStream::MarketKNN_10000_11000);
    UNIT_BCLM_SIZE1_STREAM(TMarketKNN_11000_20000BclmSize1Unit, TStream::MarketKNN_11000_20000);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketManufacturerBclmSize1Unit, TStream::YaMarketManufacturer);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketCategoryNameBclmSize1Unit, TStream::YaMarketCategoryName);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketAliasesBclmSize1Unit, TStream::YaMarketAliases);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketUrlBclmSize1Unit, TStream::YaMarketUrl);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketOfferDescriptionBclmSize1Unit, TStream::YaMarketOfferDescription);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketOfferSalesNotesBclmSize1Unit, TStream::YaMarketOfferSalesNotes);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketOfferTitleBclmSize1Unit, TStream::YaMarketOfferTitle);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketOfferUrlBclmSize1Unit, TStream::YaMarketOfferUrl);
    UNIT_BCLM_SIZE1_STREAM(TYaMarketSplitDwellTimeBclmSize1Unit, TStream::YaMarketSplitDwellTime)
    UNIT_BCLM_SIZE1_STREAM(TYaMarketOneClickBclmSize1Unit, TStream::YaMarketOneClick)
    UNIT_BCLM_SIZE1_STREAM(TYaMarketLongClickBclmSize1Unit, TStream::YaMarketLongClick)
    UNIT_BCLM_SIZE1_STREAM(TYaMarketLongClick120SqrtBclmSize1Unit, TStream::YaMarketLongClick120Sqrt)
    UNIT_BCLM_SIZE1_STREAM(TYaMarketSimpleClickBclmSize1Unit, TStream::YaMarketSimpleClick)
    UNIT_BCLM_SIZE1_STREAM(TYaMarketFirstLastClickMobileBclmSize1Unit, TStream::YaMarketFirstLastClickMobile)
    UNIT_BCLM_SIZE1_STREAM(TCorrectedCtrLongPeriodBclmSize1Unit, TStream::CorrectedCtrLongPeriod)
    UNIT_BCLM_SIZE1_STREAM(TUserQueryBclmSize1Unit, TStream::UserQuery)

    UNIT_BCLM_SIZE1_STREAM(TMarketBlueAliasOfWhiteModelBclmSize1Unit, TStream::MarketBlueAliasOfWhiteModel)
    UNIT_BCLM_SIZE1_STREAM(TMarketBlueTitleOfWhiteModelBclmSize1Unit, TStream::MarketBlueTitleOfWhiteModel)
    UNIT_BCLM_SIZE1_STREAM(TMarketBlueDescriptionOfWhiteOfferBclmSize1Unit, TStream::MarketBlueDescriptionOfWhiteOffer)
    UNIT_BCLM_SIZE1_STREAM(TMarketBlueTitleOfWhiteOfferBclmSize1Unit, TStream::MarketBlueTitleOfWhiteOffer)

    UNIT_BCLM_SIZE1_STREAM(TMarketTitleBclmSize1Unit, TStream::MarketTitle)
    UNIT_BCLM_SIZE1_STREAM(TMarketingDescriptionBclmSize1Unit, TStream::MarketingDescription)
    UNIT_BCLM_SIZE1_STREAM(TBlueMarketingDescriptionOfModelBclmSize1Unit, TStream::BlueMarketingDescriptionOfModel)
    UNIT_BCLM_SIZE1_STREAM(TBlueMicroDescriptionOfModelBclmSize1Unit, TStream::BlueMicroDescriptionOfModel)

    UNIT_BCLM_SIZE1_STREAM(TMarketCPAQueryBclmSize1Unit, TStream::MarketCPAQuery)

    UNIT_BCLM_MIX_STREAM_WEIGHTED(TVideoUnitedFactorAnnBclmMixUnit, TStream::VideoUnitedFactorAnn, THitWeight::V0)
    UNIT_BCLM_SIZE1_STREAM(TMskuOfferSearchTextBclmSize1Unit, TStream::MskuOfferSearchText)
    UNIT_BCLM_SIZE1_STREAM(TMskuOfferTitleBclmSize1Unit, TStream::MskuOfferTitle)

    UNIT_BCLM_SIZE1_STREAM(TMetadocBQRPBclmSize1Unit, TStream::MetadocBQRP)
    UNIT_BCLM_SIZE1_STREAM(TMetadocFirstClickDTXFBclmSize1Unit, TStream::MetadocFirstClickDTXF)
    UNIT_BCLM_SIZE1_STREAM(TMetadocLongClickBclmSize1Unit, TStream::MetadocLongClick)
    UNIT_BCLM_SIZE1_STREAM(TMetadocLongClickSPBclmSize1Unit, TStream::MetadocLongClickSP)
    UNIT_BCLM_SIZE1_STREAM(TMetadocOneClickBclmSize1Unit, TStream::MetadocOneClick)
    UNIT_BCLM_SIZE1_STREAM(TMetadocSimpleClickBclmSize1Unit, TStream::MetadocSimpleClick)
    UNIT_BCLM_SIZE1_STREAM(TMetadocSplitDTBclmSize1Unit, TStream::MetadocSplitDT)
    UNIT_BCLM_SIZE1_STREAM(TMetadocBQPRSampleBclmSize1Unit, TStream::MetadocBQPRSample)
    UNIT_BCLM_SIZE1_STREAM(TMetadocNHOPBclmSize1Unit, TStream::MetadocNHOP)

    UNIT_BCLM_SIZE1_STREAM(TVendorNameBclmSize1Unit, TStream::VendorName)
    UNIT_BCLM_SIZE1_STREAM(TShopNameBclmSize1Unit, TStream::ShopName)
    UNIT_BCLM_SIZE1_STREAM(TShopCategoriesBclmSize1Unit, TStream::ShopCategories)
    UNIT_BCLM_SIZE1_STREAM(TNidNamesBclmSize1Unit, TStream::NidNames)

} // NCore
} // NTextMachine

#undef UNIT_BCLM_SIZE1_STREAM
#undef UNIT_BCLM_MIX_STREAM
