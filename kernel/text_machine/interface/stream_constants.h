#pragma once

#include <kernel/lingboost/enum_map.h>
#include <kernel/lingboost/enum.h>
#include <kernel/lingboost/constants.h>

#include <kernel/country_data/countries.h>

#include <library/cpp/wordpos/wordpos.h>

#include <util/digest/multi.h>
#include <util/generic/strbuf.h>
#include <util/generic/hash.h>

namespace NLingBoost {

    struct TBaseIndexStruct {
        enum EType {
            Text = 0,
            Link = 1,
            Ann = 2,
            FactorAnn = 3,
            LinkAnn = 4,
            BaseIndexMax
        };
        static const size_t Size = BaseIndexMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Text, Link, Ann, FactorAnn, LinkAnn>();
        }
    };
    using TBaseIndex = TEnumOps<TBaseIndexStruct>;
    using EBaseIndexType = TBaseIndex::EType;

    // NOTE. Layer is a technical term that
    // means "batch of hits loaded from base index".
    // We may choose to have several batches ("layers"), fetching
    // hits for smaller subset of basic blocks in each
    // subsequent tier. E.g. first load N hits for all expansions,
    // then load extra M hits only for OriginalRequest.
    struct TBaseIndexLayerStruct {
        enum EType {
            Layer0 = 0,
            Layer1 = 1,
            BaseIndexLayerMax
        };
        static const size_t Size = BaseIndexLayerMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Layer0, Layer1>();
        }
    };
    using TBaseIndexLayer = TEnumOps<TBaseIndexLayerStruct>;
    using EBaseIndexLayerType = TBaseIndexLayer::EType;

    struct TStreamStruct {
        // CAUTION. Constants from this enum
        // are used in proto format as raw integers.
        // Changing value of enum constant may break
        // existing proto pools that contain hits data.
        enum EType {
            Url = 0,
            Title = 1,
            Body = 2,
            CatalogueYandex = 3,
            CatalogueDmoz = 4,
            LinksInternal = 5,
            LinksExternalDpr = 6,
            LinksExternalTic = 7,
            CorrectedCtr = 8,
            WikiTitle = 9,
            SameQueryReturnFrcBrowser = 10,
            SamplePeriodDayFrc = 11,
            OneClick = 12,
            LongClick = 13,
            SplitDwellTime = 14,
            BrowserPageRank = 15,
            YabarVisits = 16,
            YabarTime = 17,
            SimpleClick = 18,
            RandomLogDBM35 = 19,
            CorrectedCtrXFactor = 20,
            PopularSeFrcBrowser = 21,
            LongClickSP = 22,
            UrlQuery = 23,
            NHopTotal = 24,
            NHopIsFinal = 25,
            BeastQ = 26,
            DoubleFrc = 27,
            BQPRSample = 28,
            OneClickFrcXfSp = 29,
            QueryDwellTime = 30,
            Onotole = 31,
            CorrectedCtrLongPeriod = 32,
            NHopSumDwellTime = 33,
            FirstClickDtXf = 34,
            MetaPolyGen8 = 37,
            LongClickMobile = 38,
            LinksExternal = 39,
            BQPR = 40,
            FirstLastClickMobile = 41,
            AvgDTWeightedByRankMobile = 42,
            //LinkAnn
            //
            Removed43 = 43,
            LinkAnnLinkInternal = 44,
            LinkAnnLinkExternal = 45,
            LinkAnnLinkExternalDpr = 46,
            LinkAnnIndicator = 47,

            //Removed48 = 48,
            //Removed49 = 49,
            //Removed50 = 50,

            LinkAnnFloatMultiplicity = 51,

            VpcgCorrectedClicksSlowLongPeriod = 60,
            FullText = 61,
            WebVideoPCtrNew = 62,
            AliceMusicTrackTitle = 63,
            AliceMusicArtistName = 64,
            AliceMusicAlbumTitle = 65,
            AliceMusicTrackArtistNames = 67,
            AliceMusicTrackAlbumTitle = 68,
            AliceMusicTrackLyrics = 69,

            // Images
            //
            ImagesCVTags = 100,
            ImagesGoXFRC = 101,
            ImagesOCR = 102,
            ImagesRightXFRC = 103,
            ImagesGoClicks = 104,
            ImagesYaClicks = 105,
            ImagesTolokaOk = 106,
            ImagesTolokaBad = 107,
            ImagesTitle = 108,
            ImagesClickSimilarity = 109,
            ImagesOCRv2 = 110,
            ImagesRTClicks = 111,

            // AdvMachine
            //
            AdvBannerPhraseRsyaCtr = 150,
            AdvBannerPhraseRsyaFrc = 151,
            AdvBannerPhraseRsyaHistoryCpc = 152,
            AdvBannerRsyaPhraseBid = 153,
            AdvBannerRsyaQueryBid = 154,
            AdvBannerRsyaQueryCtr = 155,
            AdvBannerRsyaQueryFrc = 156,
            AdvBannerRsyaQueryHistoryCpc = 157,
            AdvBannerSpyLogTitle = 158,
            AdvBannerTitle = 159,
            AdvLandingPageTitle = 160,
            AdvBannerCommQueriesCtr = 161,
            AdvBannerCommQueriesFrc = 162,
            AdvBannerCommQueriesXfCtr = 163,
            AdvBannerPhraseCtr = 164,
            AdvBannerPhraseHistoryCpc = 165,
            AdvBannerText = 166,
            AdvBannerPhraseHistoryCpm = 167,
            AdvLandingPageText = 168,
            AdvBannerCommQueriesBid = 169,
            AdvBannerCommQueriesHistoryCpc = 170,
            AdvBannerPhraseBid = 171,
            AdvBannerQueryCostFr = 172,
            AdvBannerOriginalPhrase = 173,
            AdvBannerOriginalPhraseBid = 174,
            AdvBannerOriginalPhraseBidToMaxBid = 175,
            UserQuery = 176,
            AdvMachine1 = 177,
            AdvMachine2 = 178,
            AdvMachine3 = 179,
            AdvMachine4 = 180,
            AdvMachine5 = 181,
            AdvMachine6 = 182,
            AdvMachine7 = 183,
            AdvMachine8 = 184,

            // Video
            //
            VideoQusm = 200,
            //Removed201 = 201,
            VideoMaxClicksPercent = 202,
            VideoPctrNew = 203,
            VideoNHop = 204,
            VideoCorrectedClicksFrc = 205,
            VideoCorrectedClicksUFrc = 206,
            VideoUnitedFactorAnn = 207,
            VideoSpeechToText = 208,
            VideoOcr = 209,

            // Geo
            //
            MapsShowQFRC = 300,
            MapsClickQFRC = 301,
            MapsOrgAddress = 302,
            MapsOrgName = 303,
            MapsOrgChainName = 304,

            MobileMapsClickQFRC = 305,
            MobileMapsClickSQFRC = 306,
            MobileMapsClickCTR = 307,

            SerpMapsClickQFRC = 308,
            SerpMapsClickSOFRC = 309,
            SerpMapsClickSQFRC = 310,

            LocatedAtOrgName = 311,
            FacultyName = 312,
            HospitalName = 313,
            OrgSummary = 314,

            // Market
            //
            MarketModelAlias = 400,
            MarketImageLinkData = 401,
            MarketImageAltData = 402,
            MarketImageShowsTime = 403,
            MarketImageQueryShowsTime = 404,
            MarketImageDwellTime = 405,
            MarketImageQueryDwellTime = 406,
            MarketImageDocDwellTime = 407,
            MarketImageShows = 408,
            MarketImageClicks = 409,
            MarketImageQueryClicks = 410,
            MarketDescription = 411,
            MarketWasOrdered = 412,
            MarketKNN_5000_9600 = 413,
            MarketKNN_9600_10000 = 414,
            MarketKNN_10000_11000 = 415,
            MarketKNN_11000_20000 = 416,

            // Yandex.Market.AdvMachine
            YaMarketManufacturer = 500,
            YaMarketContents = 501,
            YaMarketAliases = 502,
            YaMarketBarcode = 503,
            YaMarketBookPublisher = 504,
            YaMarketBookUisbn = 505,
            YaMarketUrl = 506,
            YaMarketBookAuthor = 507,
            YaMarketMobileManufacturer = 508,
            YaMarketOfferDescription = 509,
            YaMarketOfferSalesNotes = 510,
            YaMarketOfferTitle = 511,
            YaMarketOfferUrl = 512,
            YaMarketCategoryName = 513,
            //

            // Yandex.Market.ClickMachine
            YaMarketSplitDwellTime = 600,
            YaMarketOneClick = 601,
            YaMarketLongClick = 602,
            YaMarketLongClick120Sqrt = 603,
            YaMarketSimpleClick = 604,
            YaMarketLongClickMobile = 605,
            YaMarketFirstLastClickMobile = 606,

            MarketBlueAliasOfWhiteModel = 607,
            MarketBlueTitleOfWhiteModel = 608,

            MarketBlueDescriptionOfWhiteOffer = 609,
            MarketBlueTitleOfWhiteOffer = 610,

            MarketTitle = 611,

            MarketingDescription = 612,
            BlueMarketingDescriptionOfModel = 613,
            BlueMicroDescriptionOfModel = 614,

            MarketCPAQuery = 615,

            // Music
            MusicFirstClick = 700,
            MusicLastClick = 701,
            MusicSingleClick = 702,
            MusicLongClick = 703,

            MskuOfferSearchText = 704,
            MskuOfferTitle = 705,
            MetadocBQRP = 706,
            MetadocFirstClickDTXF = 707,
            MetadocLongClick = 708,
            MetadocLongClickSP = 709,
            MetadocOneClick = 710,
            MetadocSimpleClick = 711,
            MetadocSplitDT = 712,
            MetadocBQPRSample = 713,
            MetadocNHOP = 714,

            VendorName = 715,
            ShopName = 716,
            ShopCategories = 717,
            NidNames = 718,

            //
            StreamMax
        };

        static const size_t Size = StreamMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Url, Title, Body, CatalogueYandex, CatalogueDmoz,
                LinksInternal, LinksExternalDpr, LinksExternalTic, CorrectedCtr, WikiTitle,
                SameQueryReturnFrcBrowser, SamplePeriodDayFrc, OneClick, LongClick, SplitDwellTime,
                BrowserPageRank, YabarVisits, YabarTime, SimpleClick, RandomLogDBM35, CorrectedCtrXFactor,
                PopularSeFrcBrowser, LongClickSP, UrlQuery, NHopTotal, NHopIsFinal, BeastQ, DoubleFrc,
                BQPRSample, OneClickFrcXfSp, QueryDwellTime, Onotole, CorrectedCtrLongPeriod, NHopSumDwellTime, FirstClickDtXf,
                MetaPolyGen8, LongClickMobile, LinksExternal, BQPR, FirstLastClickMobile, AvgDTWeightedByRankMobile,

                Removed43, LinkAnnLinkInternal, LinkAnnLinkExternal, LinkAnnLinkExternalDpr, LinkAnnIndicator, LinkAnnFloatMultiplicity,
                VpcgCorrectedClicksSlowLongPeriod, FullText, WebVideoPCtrNew,

                AliceMusicTrackTitle, AliceMusicArtistName, AliceMusicAlbumTitle, AliceMusicTrackArtistNames, AliceMusicTrackAlbumTitle, AliceMusicTrackLyrics,

                ImagesCVTags, ImagesGoXFRC, ImagesOCR, ImagesRightXFRC, ImagesGoClicks,
                ImagesYaClicks, ImagesTolokaOk, ImagesTolokaBad, ImagesTitle, ImagesClickSimilarity, ImagesOCRv2, ImagesRTClicks,

                AdvBannerPhraseRsyaCtr, AdvBannerPhraseRsyaFrc, AdvBannerPhraseRsyaHistoryCpc, AdvBannerRsyaPhraseBid, AdvBannerRsyaQueryBid,
                AdvBannerRsyaQueryCtr, AdvBannerRsyaQueryFrc, AdvBannerRsyaQueryHistoryCpc, AdvBannerSpyLogTitle,
                AdvBannerTitle, AdvLandingPageTitle, AdvBannerCommQueriesCtr, AdvBannerCommQueriesFrc, AdvBannerCommQueriesXfCtr,
                AdvBannerPhraseCtr, AdvBannerPhraseHistoryCpc, AdvBannerText, AdvBannerPhraseHistoryCpm, AdvLandingPageText,
                AdvBannerCommQueriesBid, AdvBannerCommQueriesHistoryCpc, AdvBannerPhraseBid, AdvBannerQueryCostFr,
                AdvBannerOriginalPhrase, AdvBannerOriginalPhraseBid, AdvBannerOriginalPhraseBidToMaxBid, UserQuery,
                AdvMachine1, AdvMachine2, AdvMachine3, AdvMachine4, AdvMachine5, AdvMachine6, AdvMachine7, AdvMachine8,

                VideoQusm, VideoMaxClicksPercent, VideoPctrNew, VideoNHop, VideoCorrectedClicksFrc, VideoCorrectedClicksUFrc,
                VideoUnitedFactorAnn, VideoSpeechToText, VideoOcr,

                MapsShowQFRC, MapsClickQFRC,
                MapsOrgAddress, MapsOrgName, MapsOrgChainName,
                MobileMapsClickQFRC,MobileMapsClickSQFRC,MobileMapsClickCTR,
                SerpMapsClickQFRC, SerpMapsClickSOFRC, SerpMapsClickSQFRC,
                LocatedAtOrgName, FacultyName, HospitalName, OrgSummary,

                MarketModelAlias, MarketImageLinkData, MarketImageAltData, MarketImageShowsTime, MarketImageQueryShowsTime, MarketImageDwellTime, MarketImageQueryDwellTime,
                MarketImageDocDwellTime, MarketImageShows, MarketImageClicks, MarketImageQueryClicks, MarketDescription, MarketWasOrdered,
                MarketKNN_5000_9600, MarketKNN_9600_10000, MarketKNN_10000_11000, MarketKNN_11000_20000,

                YaMarketManufacturer, YaMarketContents, YaMarketAliases, YaMarketBarcode, YaMarketBookPublisher,
                YaMarketBookUisbn, YaMarketUrl, YaMarketBookAuthor, YaMarketMobileManufacturer,
                YaMarketOfferDescription, YaMarketOfferSalesNotes, YaMarketOfferTitle, YaMarketOfferUrl, YaMarketCategoryName,
                YaMarketSplitDwellTime, YaMarketOneClick, YaMarketLongClick, YaMarketLongClick120Sqrt, YaMarketSimpleClick,
                YaMarketLongClickMobile, YaMarketFirstLastClickMobile,

                MarketBlueAliasOfWhiteModel, MarketBlueTitleOfWhiteModel, MarketBlueDescriptionOfWhiteOffer, MarketBlueTitleOfWhiteOffer,

                MarketTitle,

                MarketingDescription, BlueMarketingDescriptionOfModel, BlueMicroDescriptionOfModel, MarketCPAQuery,

                MusicFirstClick, MusicLastClick, MusicSingleClick, MusicLongClick, MskuOfferSearchText, MskuOfferTitle,

                MetadocBQRP, MetadocFirstClickDTXF, MetadocLongClick, MetadocLongClickSP, MetadocOneClick, MetadocSimpleClick, MetadocSplitDT, MetadocBQPRSample, MetadocNHOP,

                VendorName, ShopName, ShopCategories, NidNames
                >();
        }

        static TStringBuf GetFullName() {
            return TStringBuf("::NLingBoost::TStream");
        }
    };
    using TStream = TEnumOps<TStreamStruct>;
    using TStreamTypeDescr = TEnumTypeDescr<TStreamStruct>;
    using EStreamType = TStream::EType;

    // Update this function after adding new streams
    EBaseIndexType GetBaseIndexByStream(EStreamType stream);

    namespace NPrivate {
        void FillBaseIndexByStreamMap(TCompactEnumMap<TStream, EBaseIndexType>& map);

        class TBaseIndexByStreamMap
            : public TCompactEnumMap<TStream, EBaseIndexType>
        {
        public:
            TBaseIndexByStreamMap() {
                FillBaseIndexByStreamMap(*this);
            }
        };
    }

    inline EBaseIndexType GetBaseIndexByStream(EStreamType stream) {
        static const NPrivate::TBaseIndexByStreamMap table;
        Y_ASSERT(table[stream] != TBaseIndex::BaseIndexMax);
        return table[stream];
    }
}

bool FromString(const TStringBuf& text, NLingBoost::EStreamType& value);
bool FromString(const TStringBuf& text, NLingBoost::EBaseIndexType& value);
bool FromString(const TStringBuf& text, NLingBoost::EBaseIndexLayerType& value);
