/// author@ vvp@ Victor Ploshikhin
/// created: Oct 10, 2013 8:21:50 AM
/// see: ROBOT-2993

#include "data.h"


namespace NIndexAnn {

template<EDataType DataType>
static bool ExtractAnnDataImpl(const TRegionData& /*src*/, TRegionData& /*dst*/)
{
    static_assert(DataType == -1, "define data extractor for your DataType");
    return false;
}

#define DEFINE_COPY_STREAM_DATA(DataType, Field) \
template<> \
bool ExtractAnnDataImpl<DataType>(const TRegionData& src, TRegionData& dst) \
{ \
    if (src.Has##Field()) { \
        *dst.Mutable##Field() = src.Get##Field(); \
        return true; \
    } \
    return false; \
}

#define DEFINE_COPY_STREAM_DATA_CZ(DataType, Field) \
template<> \
bool ExtractAnnDataImpl<DataType>(const TRegionData& src, TRegionData& dst) \
{ \
    if (src.Has##Field() && src.Get##Field() > 1e-6) { \
        *dst.Mutable##Field() = src.Get##Field(); \
        return true; \
    } \
    return false; \
}

#define DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DataType, Field, SubField) \
template<> \
bool ExtractAnnDataImpl<DataType>(const TRegionData& src, TRegionData& dst) \
{ \
    if (src.Has##Field() && src.Get##Field().Has##SubField()) { \
        *(dst.Mutable##Field()->Mutable##SubField()) = src.Get##Field().Get##SubField(); \
        return true; \
    } \
    return false; \
}

#define DEFINE_COPY_STREAM_DATA_SUB(DataType, Field, SubField) \
template<> \
bool ExtractAnnDataImpl<DataType>(const TRegionData& src, TRegionData& dst) \
{ \
    if (src.Has##Field() && src.Get##Field().Has##SubField()) { \
        dst.Mutable##Field()->Set##SubField(src.Get##Field().Get##SubField()); \
        return true; \
    } \
    return false; \
}

// Check zero.
#define DEFINE_COPY_STREAM_DATA_SUB_CZ(DataType, Field, SubField) \
template<> \
bool ExtractAnnDataImpl<DataType>(const TRegionData& src, TRegionData& dst) \
{ \
    if (src.Has##Field() && src.Get##Field().Has##SubField() && src.Get##Field().Get##SubField() > 1.0e-6) { \
        dst.Mutable##Field()->Set##SubField(src.Get##Field().Get##SubField()); \
        return true; \
    } \
    return false; \
}

DEFINE_COPY_STREAM_DATA(DT_ADPART, DummyData);
DEFINE_COPY_STREAM_DATA(DT_UQ, UQ);
DEFINE_COPY_STREAM_DATA(DT_NHOP, NHop);
DEFINE_COPY_STREAM_DATA(DT_WT, WT);
DEFINE_COPY_STREAM_DATA(DT_BEAST_Q, BeastQ);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_6, DummyData);
DEFINE_COPY_STREAM_DATA(DT_CORRECTED_CLICKS, CorrectedClicks);
DEFINE_COPY_STREAM_DATA(DT_NHOP_VIDEO, NHopVideo);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BQPR, BrowserPageRank, Value);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SPLIT_DT, ClickMachine, SplitDt);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ONE_CLICK, ClickMachine, OneClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LONG_CLICK, ClickMachine, LongClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LONG_CLICK_SP, ClickMachine, LongClickSP);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SIMPLE_CLICK, ClickMachine, SimpleClick);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_20, DummyData);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_31, DummyData);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_DOUBLE_FRC, ClickMachine, DoubleFrc);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_QUERY_DWELL_TIME, ClickMachine, QueryDwellTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_CORRECTED_CTR_LONG_PERIOD, ClickMachine, CorrectedCtrLongPeriod);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_EXP, Experiment, Weight);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_CORRECTED_CTR, UserQueryUrl, CorrectedCtr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SAMPLE_PERIOD, UserQueryUrl, SamplePeriodDayFrc);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YABAR_VISITS, UserQueryUrl, YabarVisits);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YABAR_TIME, UserQueryUrl, YabarTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_CORRECTED_CTR_XFACTOR, UserQueryUrl, CorrectedCtrXFactor);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_RANDOM_LOG_DBM35, RandomLogDBM35, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_POPULAR_SE_FRC_BROWSER, PopularSeFrcBrowser, Value);

DEFINE_COPY_STREAM_DATA(DT_BANNER_MINUS, BannerMinusWords);
DEFINE_COPY_STREAM_DATA(DT_BANNER_TITLE, BannerTitle);
DEFINE_COPY_STREAM_DATA(DT_BANNER_TEXT, BannerText);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_BID, BannerPhrase, Bid);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_PHRASE_BID, BannerRsyaPhrase, Bid);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_HISTORY_CPM, BannerPhrase, HistoryCPM);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_HISTORY_CPC, BannerPhrase, HistoryCPC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_CTR, BannerPhrase, Ctr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_CLICKS, BannerPhrase, TotalClicks);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_FRC, BannerPhrase, Frc);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_XF_CTR, BannerPhraseXFactor, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_CTR, BannerQuery, Ctr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_FRC, BannerQuery, Frc);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_CLICKS, BannerQuery, TotalClicks);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_GUARANTEE_CLICKS, BannerGuaranteeQuery, TotalClicks);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_XF_CTR, BannerCommQueriesXF, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_RSYA_CTR, BannerRsyaPhrase, Ctr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_RSYA_FRC, BannerRsyaPhrase, Frc);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_RSYA_CLICKS, BannerRsyaPhrase, TotalClicks);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_RSYA_HISTORY_CPC, BannerRsyaPhrase, HistoryCPC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_SPY_LOG_TITLE, BannerSpyLogTitle, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LANDING_PAGE_TITLE, LandingPageTitle, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LANDING_PAGE_TEXT, LandingPageText, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LANDING_PAGE_LISTS, LandingPageLists, Value);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_QUSM, VideoQUSM, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BQPR_SAMPLE, BrowserPageRankSample, Value);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ONE_CLICK_FRC_XF_SP, OneClickFrcXfSP, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_NHOP_SUM_DWELL_TIME, NHopSumDwellTime, Value);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_54, DummyData);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_MAX_CLICKS_PERCENT, VideoMaxClicksPercent, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_PCTR_NEW, VideoPCtrNew, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_PLAYER_VIEW_TIME_MAX_LOKI, VideoPVTMLoki, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_GUARANTEE_BID, BannerGuaranteePhrase, Bid);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_GUARANTEE_CTR, BannerGuaranteePhrase, Ctr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_GUARANTEE_HISTORY_CPC, BannerGuaranteePhrase, HistoryCPC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_PHRASE_GUARANTEE_CLICKS, BannerGuaranteePhrase, TotalClicks);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_GUARANTEE_CTR, BannerGuaranteeQuery, Ctr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_FIRST_CLICK_DT_XF, FirstClickDtXf, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_QTS, VideoQTS, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_BID, BannerQuery, Bid);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_COMM_QUERIES_HISTORY_CPC, BannerQuery, HistoryCPC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_QUERY_BID, BannerRsyaQuery, Bid);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_QUERY_CTR, BannerRsyaQuery, Ctr);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_QUERY_FRC, BannerRsyaQuery, Frc);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_QUERY_HISTORY_CPC, BannerRsyaQuery, HistoryCPC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_QUERY_CLICKS_FR, BannerQueryClicksFr, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_QUERY_CLICKS_FR, BannerRsyaQueryClicksFr, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_QUERY_COST_FR, BannerQueryCostFr, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_RSYA_QUERY_COST_FR, BannerRsyaQueryCostFr, Value);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_1, DummyData);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_2, DummyData);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_ORIGINAL_PHRASE, BannerOriginalPhrase, Enable);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_ORIGINAL_PHRASE_BID, BannerOriginalPhrase, Bid);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_BANNER_ORIGINAL_PHRASE_BID_TO_MAX_BID, BannerOriginalPhrase, BidToMaxBid);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LONG_CLICK_MOBILE, ClickMachine, LongClickMobile);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_FIRST_LAST_CLICK_MOBILE, ClickMachine, FirstLastClickMobile);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_AVG_DT_WEIGHTED_BY_RANK_MOBILE, ClickMachine, AvgDTWeightedByRankMobile);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_RELATED_TERM, VideoRelatedTerm, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_WIDE_TERM, VideoWideTerm, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEOHUB_TERM, VideohubTerm, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SERNAMECRC, SerNameCRC, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SERNAMECRC_PLUS_SEASON, SerNameCRCPlusSeason, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_SPEECH_TO_TEXT, VideoSpeechToText, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_OCR, VideoOcr, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_CLICK_SIM, VideoClickSim, Value);

DEFINE_COPY_STREAM_DATA(DT_META_TAG_SENTENCE, MetaTagSentence);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MAPS_SHOW_QFRC, MapsStreams, ShowQFRC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MAPS_CLICK_QFRC, MapsStreams, ClickQFRC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MAPS_ORG_ADDRESS, MapsStreams, OrgAddress);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MAPS_ORG_NAME, MapsStreams, OrgName);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MAPS_ORG_CHAIN_NAME, MapsStreams, OrgChainName);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MOBILE_MAPS_CLICK_QFRC, MapsStreams, MobileClickQFRC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MOBILE_MAPS_CLICK_SQFRC, MapsStreams, MobileClickSQFRC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MOBILE_MAPS_CLICK_CTR, MapsStreams, MobileClickCTR);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SERP_MAPS_CLICK_QFRC, MapsStreams, SerpMapsClickQFRC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SERP_MAPS_CLICK_SOFRC, MapsStreams, SerpMapsClickSOFRC);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_SERP_MAPS_CLICK_SQFRC, MapsStreams, SerpMapsClickSQFRC);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_LOCATED_AT_ORG_NAME, MapsStreams, LocatedAtOrgName);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_FACULTY_NAME, MapsStreams, FacultyName);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_HOSPITAL_NAME, MapsStreams, HospitalName);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ORG_SUMMARY, MapsStreams, OrgSummary);

// Market factors
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_MODEL_ALIAS, MarketStreams, MarketModelAlias);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_DESCRIPTION, MarketStreams, MarketDescription);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_TITLE, MarketStreams, MarketTitle);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_WAS_ORDERED, MarketStreams, WasOrdered);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_RECIPE, MarketStreams, MarketRecipe);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_MARKETING_DESCR, MarketStreams, MarketingDescription);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_BLUE_MRKT_MARKETING_DESCR, MarketStreams, BlueMarketingDescriptionOfModel);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_BLUE_MRKT_MICRO_MODEL_DESCR, MarketStreams, BlueMicroDescriptionOfModel);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_MICRO_MODEL_DESCR, MarketStreams, MicroDescriptionOfModel);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_IMG_LINK_DATA, MarketStreams, ImageLinkData);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_IMG_ALT_DATA, MarketStreams, ImageAltData);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_SHOWS_TIME, MarketStreams, ImageShowsTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_QUERY_SHOWS_TIME, MarketStreams, ImageQueryShowsTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_DWELL_TIME, MarketStreams, ImageDwellTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_QUERY_DWELL_TIME, MarketStreams, ImageQueryDwellTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_DOC_DWELL_TIME, MarketStreams, ImageDocDwellTime);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_SHOWS, MarketStreams, ImageShows);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_CLICKS, MarketStreams, ImageClicks);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_IMG_QUERY_CLICKS, MarketStreams, ImageQueryClicks);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_CPA_QUERY, MarketStreams, CPAQuery);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_VENDOR_NAME, MarketStreams, VendorName);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_SHOP_NAME, MarketStreams, ShopName);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_SHOP_CATEGORIES, MarketStreams, ShopCategories);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_NID_NAMES, MarketStreams, NidNames);

// KNN Market streams
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_KNN_5000_9600, MarketStreams, KNN_5000_9600);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_KNN_9600_10000, MarketStreams, KNN_9600_10000);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_KNN_10000_11000, MarketStreams, KNN_10000_11000);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_KNN_11000_20000, MarketStreams, KNN_11000_20000);

// Market Experimental streams
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_UNO_EXP, MarketExpStreams, MarketUnoExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_DOS_EXP, MarketExpStreams, MarketDosExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_TRES_EXP, MarketExpStreams, MarketTresExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_CUATRO_EXP, MarketExpStreams, MarketCuatroExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_CINCO_EXP, MarketExpStreams, MarketCincoExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_SEIS_EXP, MarketExpStreams, MarketSeisExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_SIETE_EXP, MarketExpStreams, MarketSieteExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_OCHO_EXP, MarketExpStreams, MarketOchoExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_NUEVE_EXP, MarketExpStreams, MarketNueveExp);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_DIEZ_EXP, MarketExpStreams, MarketDiezExp);

// Streams for Blue Market
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_BLUE_MRKT_DESCRIPTION, MarketStreams, BlueDescriptionOfWhiteOffer);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_BLUE_MRKT_TITLE, MarketStreams, BlueTitleOfWhiteOffer);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_BLUE_MRKT_MODEL_TITLE, MarketStreams, BlueTitleOfWhiteModel);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_BLUE_MRKT_MODEL_ALIAS, MarketStreams, BlueAliasOfWhiteModel);

// Market msku streams
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_MSKU_OFFER_TITLE, MarketStreams, MskuOfferTitle);
DEFINE_COPY_DUMMY_STREAM_DATA_SUB(DT_MRKT_MSKU_OFFER_SEARCH_TEXT, MarketStreams, MskuOfferSearchText);
// Market metadoc web streams
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_BQPR, MarketMetadocStreams, BrowserPageRank);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_FIRST_CLICK_DT_XF, MarketMetadocStreams, FirstClickDtXf);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_LONG_CLICK, MarketMetadocStreams, LongClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_LONG_CLICK_SP, MarketMetadocStreams, LongClickSP);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_ONE_CLICK, MarketMetadocStreams, OneClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_SIMPLE_CLICK, MarketMetadocStreams, SimpleClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_SPLIT_DT, MarketMetadocStreams, SplitDt);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MRKT_METADOC_BQPR_SAMPLE, MarketMetadocStreams, BrowserPageRankSample);
DEFINE_COPY_STREAM_DATA(DT_MRKT_METADOC_NHOP, MarketMetadocNHop);

// Yandex.Market.AdvMachine streams BEGIN
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_MANUFACTURER, YaMarketManufacturer);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_CATEGORY_NAME, YaMarketCategoryName);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_CONTENTS, YaMarketContents);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_ALIASES, YaMarketAliases);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_BARCODE, YaMarketBarcode);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_BOOK_PUBLISHER, YaMarketBookPublisher);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_BOOK_UISBN, YaMarketBookUisbn);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_URL, YaMarketUrl);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_BOOK_AUTHOR, YaMarketBookAuthor);
DEFINE_COPY_STREAM_DATA(DT_YA_MARKET_MOBILE_MANUFACTURER, YaMarketMobileManufacturer);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_OFFER_DESCRIPTION, YaMarketOfferDescription, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_OFFER_SALES_NOTES, YaMarketOfferSalesNotes, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_OFFER_TITLE, YaMarketOfferTitle, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_OFFER_URL, YaMarketOfferUrl, Value);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_SPLIT_DT, MarketClickMachine, SplitDt);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_ONE_CLICK, MarketClickMachine, OneClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_LONG_CLICK, MarketClickMachine, LongClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_LONG_CLICK_120_SQRT, MarketClickMachine, LongClick120Sqrt);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_SIMPLE_CLICK, MarketClickMachine, SimpleClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_LONG_CLICK_MOBILE, MarketClickMachine, LongClickMobile);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_YA_MARKET_FIRST_LAST_CLICK_MOBILE, MarketClickMachine, FirstLastClickMobile);
// Yandex.Market.AdvMachine streams END

DEFINE_COPY_STREAM_DATA(DT_UNUSED_89, DummyData);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_90, DummyData);
DEFINE_COPY_STREAM_DATA(DT_UNUSED_91, DummyData);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_1, AdvMachine, Val1);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_2, AdvMachine, Val2);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_3, AdvMachine, Val3);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_4, AdvMachine, Val4);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_5, AdvMachine, Val5);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_6, AdvMachine, Val6);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_7, AdvMachine, Val7);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ADV_MACHINE_8, AdvMachine, Val8);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VPCG_CORRECTED_CLICKS_SLP, ClickMachine, VpcgCorrectedClicksSLP);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ALICE_MUSIC_STREAMS_TRACK_TITLE, AliceMusicStreams, TrackTitle);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ALICE_MUSIC_STREAMS_ARTIST_NAME, AliceMusicStreams, ArtistName);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ALICE_MUSIC_STREAMS_ALBUM_TITLE, AliceMusicStreams, AlbumTitle);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ALICE_MUSIC_STREAMS_TRACK_ARTIST_NAMES, AliceMusicStreams, TrackArtistNames);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ALICE_MUSIC_STREAMS_TRACK_ALBUM_TITLE, AliceMusicStreams, TrackAlbumTitle);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_ALICE_MUSIC_STREAMS_TRACK_LYRICS, AliceMusicStreams, TrackLyrics);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MUSIC_STREAMS_FIRST_CLICK, MusicStreams, FirstClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MUSIC_STREAMS_LAST_CLICK, MusicStreams, LastClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MUSIC_STREAMS_LONG_CLICK, MusicStreams, LongClick);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_MUSIC_STREAMS_SINGLE_CLICK, MusicStreams, SingleClick);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_RC_SEARCH_CLICKS_D30T5, RapidClicksStreams, SearchClicksD30T5);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_GENRE, KinopoiskStreams, Genre);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_CAST, KinopoiskStreams, Cast);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_CHARACTER, KinopoiskStreams, Character);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_KEYWORD, KinopoiskStreams, Keyword);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_AWARD, KinopoiskStreams, Award);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_TAG, KinopoiskStreams, Tag);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_I2I_SIMILARITY, KinopoiskStreams, Item2ItemSimilarity);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_HANDMADE_SIMILARITY, KinopoiskStreams, HandmadeSimilarity);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_KP_PERSON_CHARACTER, KinopoiskStreams, PersonCharacter);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_PRS_POSITION, PrsPosition, Value);

DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_HASHTAGS, VideoHashtags, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_TRANSLATED_TITLE, VideoTranslatedTitle, Value);
DEFINE_COPY_STREAM_DATA_SUB_CZ(DT_VIDEO_TRANSLATED_TEXT, VideoTranslatedText, Value);

#undef DEFINE_COPY_STREAM_DATA
#undef DEFINE_COPY_STREAM_DATA_CZ
#undef DEFINE_COPY_STREAM_DATA_SUB
#undef DEFINE_COPY_STREAM_DATA_SUB_CZ

template<EDataType DataType>
bool InvokeExtract(const TRegionData& src,
                   const IFeaturesProfile& profile,
                   TRegionData& dst)
{
    bool res = InvokeExtract<static_cast<EDataType>(DataType - 1)>(src, profile, dst);
    if (profile.IsEssential(DataType) && ExtractAnnDataImpl<DataType>(src, dst)) {
        res = true;
    }
    return res;
}

template<>
bool InvokeExtract<DT_NONE>(const TRegionData& /*src*/,
                   const IFeaturesProfile& /*profile*/,
                   TRegionData& /*dst*/)
{
    return false;
}

static
bool ExtractAnnData(const TRegionData& src,
                    const IFeaturesProfile& profile,
                    TRegionData& dst)
{
    dst.Clear();
    if (InvokeExtract<static_cast<EDataType>(DT_NUM_TYPES - 1)>(src, profile, dst)) {
        dst.SetRegion(src.GetRegion());
        return true;
    }
    return false;
}

template<EDataType DataType>
void InvokeEnumerate(const TRegionData& src, THashSet<EDataType>& features)
{
    InvokeEnumerate<static_cast<EDataType>(DataType - 1)>(src, features);

    TRegionData dst;
    if (ExtractAnnDataImpl<DataType>(src, dst)) {
        features.insert(DataType);
    }
}

template<>
void InvokeEnumerate<DT_NONE>(const TRegionData& /*src*/, THashSet<EDataType>& /*features*/)
{
}

void EnumerateFeatures(const TAnnotationRec& src, TVector<EDataType>& features)
{
    THashSet<EDataType> featuresSet;

    for (const auto& currData : src.GetData()) {
        InvokeEnumerate<static_cast<EDataType>(DT_NUM_TYPES - 1)>(currData, featuresSet);
    }

    features.clear();
    features.resize(featuresSet.size());
    Copy(featuresSet.begin(), featuresSet.end(), features.begin());
}

static
bool ExtractAnnRec(const TAnnotationRec& src,
                   const IFeaturesProfile& profile,
                   TAnnotationRec& dst)
{
    dst.Clear();
    bool ok = false;

    TRegionData nextData;

    for (size_t i = 0; i < src.DataSize(); ++i) {
        const TRegionData& currData = src.GetData(i);
        if (ExtractAnnData(currData, profile, nextData)) {
            dst.AddData()->CopyFrom(nextData);
            ok = true;
        }
    }

    if (ok) {
        *dst.MutableText() = src.GetText();
        if (src.HasTextLanguage()) {
            dst.SetTextLanguage(src.GetTextLanguage());
        }
    }

    return ok;
}

static
bool ExtractAnnSiteData(const TIndexAnnSiteData& src,
                        const IFeaturesProfile& profile,
                        TIndexAnnSiteData& dst)
{
    bool ok = false;
    dst.Clear();

    TAnnotationRec nextRec;

    for (size_t i = 0; i < src.RecsSize(); ++i) {
        const TAnnotationRec& currRec = src.GetRecs(i);
        if (ExtractAnnRec(currRec, profile, nextRec)) {
            dst.AddRecs()->CopyFrom(nextRec);
            ok = true;
        }
    }

    return ok;
}

THashMap<size_t, size_t> GetAvailableTypes()
{
    return TDataTypeConcern::Instance().AvialiableTypes;
}

bool FilterEssentialFeatures(const IFeaturesProfile& profile, const TAnnotationRec& src, TAnnotationRec& dst)
{
    return ExtractAnnRec(src, profile, dst);
}

bool FilterEssentialFeatures(const IFeaturesProfile& profile, const TIndexAnnSiteData& src, TIndexAnnSiteData& dst)
{
    return ExtractAnnSiteData(src, profile, dst);
}

} // namespace NIndexAnn

// Commented out by order of SEARCH-1699
#if 0
template<>
void Out<NIndexAnn::EDataType>(IOutputStream& os, TTypeTraits<NIndexAnn::EDataType>::TFuncParam n) {
    os << NIndexAnn::TDataTypeConcern::Instance().GetName(n);
}
#endif
