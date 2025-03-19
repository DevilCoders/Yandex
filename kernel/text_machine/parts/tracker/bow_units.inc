#define UNIT_BAG_OF_WORDS_STREAM_LIST(UnitNamePrefix, ...) \
    MACHINE_PARTS(Tracker) { \
        using UnitNamePrefix##Group = TBasicBagOfWordsGroup<__VA_ARGS__>; \
        using UnitNamePrefix##Family =  UnitNamePrefix##Group::TBasicBagOfWordsFamily; \
        template <typename Accumulator> \
        using UnitNamePrefix##Stub = UnitNamePrefix##Group::TBasicBagOfWordsStub<Accumulator>; \
        \
        UNIT_FAMILY_REGISTER( \
            Tracker, \
            UnitNamePrefix ## Family, \
            TBasicBagOfWordsFamilyInfo, \
            std::initializer_list<int>{__VA_ARGS__}); \
    }

namespace NTextMachine {
namespace NCore {
    UNIT_BAG_OF_WORDS_STREAM_LIST(TFieldSetBagOfWords, TStream::Title, TStream::Body, TStream::LongClick, TStream::LongClickSP, TStream::SimpleClick)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TTitleBagOfWords, TStream::Title)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TBodyBagOfWords, TStream::Body)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TTextBagOfWords, TStream::Title, TStream::Body)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TLongClickBagOfWords, TStream::LongClick)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TSimpleClickBagOfWords, TStream::SimpleClick)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TLongClickSPBagOfWords, TStream::LongClickSP)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TUrlBagOfWords, TStream::Url)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TCorrectedCtrBagOfWords, TStream::CorrectedCtr)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TOneClickBagOfWords, TStream::OneClick)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TBrowserPageRankBagOfWords, TStream::BrowserPageRank)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TSplitDwellTimeBagOfWords, TStream::SplitDwellTime)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TRandomLogDbm35BagOfWords, TStream::RandomLogDBM35)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TSamplePeriodDayFrcBagOfWords, TStream::SamplePeriodDayFrc)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TYabarVisitsBagOfWords, TStream::YabarVisits)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TYabarTimeBagOfWords, TStream::YabarTime)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TCorrectedCtrXFactorBagOfWords, TStream::CorrectedCtrXFactor)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TPopularSeFrcBrowserBagOfWords, TStream::PopularSeFrcBrowser)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TDoubleFrcBagOfWords, TStream::DoubleFrc)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TBQPRSampleBagOfWords, TStream::BQPRSample)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TOneClickFrcXfSpBagOfWords, TStream::OneClickFrcXfSp)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TQueryDwellTimeBagOfWords, TStream::QueryDwellTime)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoQusmBagOfWords, TStream::VideoQusm)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoMaxClicksPercentBagOfWords, TStream::VideoMaxClicksPercent)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoPctrNewBagOfWords, TStream::VideoPctrNew)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoNHopBagOfWords, TStream::VideoNHop)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoCorrectedClicksFrcBagOfWords, TStream::VideoCorrectedClicksFrc)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoCorrectedClicksUFrcBagOfWords, TStream::VideoCorrectedClicksUFrc)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoUnitedFactorAnnBagOfWords, TStream::VideoUnitedFactorAnn)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoSpeechToTextBagOfWords, TStream::VideoSpeechToText)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TVideoOcrBagOfWords, TStream::VideoOcr)

    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesGoXFRCBagOfWords, TStream::ImagesGoXFRC)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesOCRBagOfWords, TStream::ImagesOCR)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesRightXFRCBagOfWords, TStream::ImagesRightXFRC)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesCVTagsBagOfWords, TStream::ImagesCVTags)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesGoClicksBagOfWords, TStream::ImagesGoClicks)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesYaClicksBagOfWords, TStream::ImagesYaClicks)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesTolokaOkBagOfWords, TStream::ImagesTolokaOk)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesTolokaBadBagOfWords, TStream::ImagesTolokaBad)
    UNIT_BAG_OF_WORDS_STREAM_LIST(TImagesTitleBagOfWords, TStream::ImagesTitle)

    UNIT_BAG_OF_WORDS_STREAM_LIST(TOnotoleBagOfWords, TStream::Onotole)
} // NCore
} // NTextMachine

#undef UNIT_BAG_OF_WORDS_STREAM_LIST
