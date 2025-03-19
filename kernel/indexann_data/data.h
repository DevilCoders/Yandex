#pragma once

/// author@ vvp@ Victor Ploshikhin
/// created: Oct 3, 2013 3:51:21 PM
/// see: ROBOT-2993

#include <kernel/indexann/base/features_profile.h>
#include <kernel/indexann/protos/data.pb.h>

#include <util/system/defaults.h>
#include <util/generic/singleton.h>
#include <util/generic/bitmap.h>
#include <util/generic/hash.h>
#include <util/string/cast.h>


namespace NIndexAnn {

static_assert(sizeof(float) == sizeof(ui32), "float is required to be 4-bytes long");

#pragma pack(1)

// LEGACY {

struct TNHopIndexData {
    ui8 Total;
    ui8 IsFinal;
};

struct TCorrectedClicksIndexData {
    ui8 FRC;
    ui8 UFRC;
};

enum struct EAdvTextParts : ui8 {
    ATP_TITLE = 1,
    ATP_BODY = 2,
};

// } LEGACY

#pragma pack()

// See SEARCH-1699
typedef ui32 EDataType;
const EDataType DT_NONE = 0;
// Other 'fields' will be added by ADD_ANNOTATE_TYPE macro

enum EHostDataType {
    HDT_NHOP = 1,
};

template<unsigned int NUM>
struct Counter {
    enum { value = Counter<NUM-1>::value };
};

template<EDataType DT>
struct TStreamId2Type
{};

// We could do it with constexpr  C++11
#define COUNTER_INIT template<> struct Counter<__LINE__> { enum { value = 0 }; }
#define COUNTER_READ Counter<__LINE__>::value
#define COUNTER_INC template<> struct Counter<__LINE__> { enum { value = Counter<__LINE__-1>::value + 1}; };

#define ADD_ANNOTATE_TYPE(ID, TYPENAME, TYPESTRUCT) \
    static_assert(ID == Counter<__LINE__-1>::value + 1, "IDs must be sequential"); \
    template<> struct Counter<__LINE__> { enum { value = ID }; }; \
    static const EDataType TYPENAME = static_cast<EDataType> (ID); \
    static const TAnnDataType g_##TYPENAME##_const(TYPENAME, sizeof(TYPESTRUCT), #TYPENAME); \
    template<> \
    struct TStreamId2Type<TYPENAME> { \
        typedef TYPESTRUCT TType; \
    };


class TDataTypeConcern {
public:
    static inline const TDataTypeConcern& Instance() {
        return *Singleton<TDataTypeConcern>();
    }

    EDataType GetID(const TString& name) const {
        THashMap<TString, EDataType>::const_iterator it = Name2ID.find(name);
        return (it != Name2ID.end()) ? it->second : DT_NONE;
    }

    TString GetName(EDataType type) const {
        auto it = ID2Name.find(type);
        return it == ID2Name.end() ? "DT_" + ToString<ui32>(type) : it->second;
    }

    void RegisterDataType(EDataType type, size_t size, const TString &name) {
        if(AvialiableTypes.contains(type) || Name2ID.contains(name)) {
            return;
        }
        AvialiableTypes[type] = size;
        Name2ID[name] = type;
        ID2Name[type] = name;
    }

public:
    THashMap<size_t, size_t> AvialiableTypes;
    THashMap<TString, EDataType> Name2ID;
    THashMap<EDataType, TString> ID2Name;
};

struct TAnnDataType {
    TAnnDataType(EDataType type, size_t size, const TString &name) {
        Singleton<TDataTypeConcern>()->RegisterDataType(type, size, name);
    }
};

COUNTER_INIT;

ADD_ANNOTATE_TYPE(1,  DT_UQ,                                ui8);
ADD_ANNOTATE_TYPE(2,  DT_NHOP,                              ui16);
ADD_ANNOTATE_TYPE(3,  DT_WT,                                ui8);
ADD_ANNOTATE_TYPE(4,  DT_ADPART,                            ui8);
ADD_ANNOTATE_TYPE(5,  DT_BEAST_Q,                           ui8);
ADD_ANNOTATE_TYPE(6,  DT_UNUSED_6,                          ui8); // not used
ADD_ANNOTATE_TYPE(7,  DT_EXP,                               ui8);
ADD_ANNOTATE_TYPE(8,  DT_CORRECTED_CLICKS,                  ui16);
ADD_ANNOTATE_TYPE(9,  DT_CORRECTED_CTR,                     ui8);

ADD_ANNOTATE_TYPE(10, DT_NHOP_VIDEO,                        ui8);
ADD_ANNOTATE_TYPE(11, DT_SAMPLE_PERIOD,                     ui8);
ADD_ANNOTATE_TYPE(12, DT_ONE_CLICK,                         ui8);
ADD_ANNOTATE_TYPE(13, DT_LONG_CLICK,                        ui8);
ADD_ANNOTATE_TYPE(14, DT_SPLIT_DT,                          ui8);
ADD_ANNOTATE_TYPE(15, DT_BQPR,                              ui8);
ADD_ANNOTATE_TYPE(16, DT_YABAR_VISITS,                      ui8);
ADD_ANNOTATE_TYPE(17, DT_YABAR_TIME,                        ui8);
ADD_ANNOTATE_TYPE(18, DT_SIMPLE_CLICK,                      ui8);
ADD_ANNOTATE_TYPE(19, DT_RANDOM_LOG_DBM35,                  ui8);

ADD_ANNOTATE_TYPE(20, DT_UNUSED_20,                         ui8);
ADD_ANNOTATE_TYPE(21, DT_CORRECTED_CTR_XFACTOR,             ui8);
ADD_ANNOTATE_TYPE(22, DT_POPULAR_SE_FRC_BROWSER,            ui8);
ADD_ANNOTATE_TYPE(23, DT_LONG_CLICK_SP,                     ui8);
ADD_ANNOTATE_TYPE(24, DT_BANNER_PHRASE_BID,                 float);
ADD_ANNOTATE_TYPE(25, DT_BANNER_PHRASE_HISTORY_CPM,         float);
ADD_ANNOTATE_TYPE(26, DT_BANNER_PHRASE_HISTORY_CPC,         float);
ADD_ANNOTATE_TYPE(27, DT_BANNER_PHRASE_CTR,                 ui8);
ADD_ANNOTATE_TYPE(28, DT_BANNER_PHRASE_FRC,                 ui8);
ADD_ANNOTATE_TYPE(29, DT_BANNER_COMM_QUERIES_CTR,           ui8);

ADD_ANNOTATE_TYPE(30, DT_BANNER_COMM_QUERIES_FRC,           ui8);
ADD_ANNOTATE_TYPE(31, DT_UNUSED_31,                         ui8);
ADD_ANNOTATE_TYPE(32, DT_VIDEO_QUSM,                        ui8);
ADD_ANNOTATE_TYPE(33, DT_DOUBLE_FRC,                        ui8);
ADD_ANNOTATE_TYPE(34, DT_BANNER_MINUS,                      ui8);
ADD_ANNOTATE_TYPE(35, DT_BANNER_PHRASE_XF_CTR,              ui8);
ADD_ANNOTATE_TYPE(36, DT_BANNER_TITLE,                      ui8);
ADD_ANNOTATE_TYPE(37, DT_BANNER_TEXT,                       ui8);
ADD_ANNOTATE_TYPE(38, DT_BANNER_COMM_QUERIES_XF_CTR,        ui8);
ADD_ANNOTATE_TYPE(39, DT_BANNER_PHRASE_RSYA_CTR,            ui8);

ADD_ANNOTATE_TYPE(40, DT_BANNER_PHRASE_RSYA_FRC,            ui8);
ADD_ANNOTATE_TYPE(41, DT_BQPR_SAMPLE,                       ui8);
ADD_ANNOTATE_TYPE(42, DT_ONE_CLICK_FRC_XF_SP,               ui8);
ADD_ANNOTATE_TYPE(43, DT_QUERY_DWELL_TIME,                  ui8);
ADD_ANNOTATE_TYPE(44, DT_BANNER_PHRASE_CLICKS,              ui8);
ADD_ANNOTATE_TYPE(45, DT_BANNER_PHRASE_RSYA_CLICKS,         ui8);
ADD_ANNOTATE_TYPE(46, DT_BANNER_SPY_LOG_TITLE,              ui8);
ADD_ANNOTATE_TYPE(47, DT_LANDING_PAGE_TITLE,                ui8);
ADD_ANNOTATE_TYPE(48, DT_LANDING_PAGE_TEXT,                 ui8);
ADD_ANNOTATE_TYPE(49, DT_LANDING_PAGE_LISTS,                ui8);  // Not used. You may replace it with any other TFrac8IndexData field.

ADD_ANNOTATE_TYPE(50, DT_BANNER_RSYA_PHRASE_BID,            float);
ADD_ANNOTATE_TYPE(51, DT_CORRECTED_CTR_LONG_PERIOD,         ui8);
ADD_ANNOTATE_TYPE(52, DT_BANNER_PHRASE_RSYA_HISTORY_CPC,    float);
ADD_ANNOTATE_TYPE(53, DT_NHOP_SUM_DWELL_TIME,               ui8);

ADD_ANNOTATE_TYPE(54, DT_UNUSED_54,                         ui8);
ADD_ANNOTATE_TYPE(55, DT_VIDEO_MAX_CLICKS_PERCENT,          ui8);
ADD_ANNOTATE_TYPE(56, DT_VIDEO_PCTR_NEW,                    ui8);
ADD_ANNOTATE_TYPE(57, DT_VIDEO_PLAYER_VIEW_TIME_MAX_LOKI,   ui8);
ADD_ANNOTATE_TYPE(58, DT_BANNER_PHRASE_GUARANTEE_BID,       float);
ADD_ANNOTATE_TYPE(59, DT_BANNER_PHRASE_GUARANTEE_CTR,       ui8);

ADD_ANNOTATE_TYPE(60, DT_BANNER_PHRASE_GUARANTEE_HISTORY_CPC, float);
ADD_ANNOTATE_TYPE(61, DT_BANNER_PHRASE_GUARANTEE_CLICKS,    ui8);
ADD_ANNOTATE_TYPE(62, DT_BANNER_COMM_QUERIES_GUARANTEE_CTR, ui8);
ADD_ANNOTATE_TYPE(63, DT_FIRST_CLICK_DT_XF,                 ui8);
ADD_ANNOTATE_TYPE(64, DT_VIDEO_QTS,                         ui8);
ADD_ANNOTATE_TYPE(65, DT_BANNER_COMM_QUERIES_BID,           float);
ADD_ANNOTATE_TYPE(66, DT_BANNER_COMM_QUERIES_HISTORY_CPC,   float);
ADD_ANNOTATE_TYPE(67, DT_BANNER_RSYA_QUERY_BID,             float);
ADD_ANNOTATE_TYPE(68, DT_BANNER_RSYA_QUERY_CTR,             ui8);
ADD_ANNOTATE_TYPE(69, DT_BANNER_RSYA_QUERY_FRC,             ui8);

ADD_ANNOTATE_TYPE(70, DT_BANNER_RSYA_QUERY_HISTORY_CPC,     float);
ADD_ANNOTATE_TYPE(71, DT_LONG_CLICK_MOBILE,                 ui8);
ADD_ANNOTATE_TYPE(72, DT_FIRST_LAST_CLICK_MOBILE,           ui8);
ADD_ANNOTATE_TYPE(73, DT_AVG_DT_WEIGHTED_BY_RANK_MOBILE,    ui8);
ADD_ANNOTATE_TYPE(74, DT_VIDEO_RELATED_TERM,                float);
ADD_ANNOTATE_TYPE(75, DT_META_TAG_SENTENCE,                 ui8);
ADD_ANNOTATE_TYPE(76, DT_BANNER_COMM_QUERIES_CLICKS,           ui8);
ADD_ANNOTATE_TYPE(77, DT_BANNER_COMM_QUERIES_GUARANTEE_CLICKS, ui8);
ADD_ANNOTATE_TYPE(78, DT_BANNER_QUERY_CLICKS_FR,           ui8);
ADD_ANNOTATE_TYPE(79, DT_BANNER_RSYA_QUERY_CLICKS_FR,      ui8);

ADD_ANNOTATE_TYPE(80, DT_BANNER_QUERY_COST_FR,             ui8);
ADD_ANNOTATE_TYPE(81, DT_BANNER_RSYA_QUERY_COST_FR,        ui8);
ADD_ANNOTATE_TYPE(82, DT_MAPS_SHOW_QFRC,                   float);
ADD_ANNOTATE_TYPE(83, DT_MAPS_CLICK_QFRC,                  float);
ADD_ANNOTATE_TYPE(84, DT_MAPS_ORG_ADDRESS,                 ui8);
ADD_ANNOTATE_TYPE(85, DT_MAPS_ORG_NAME,                    ui8);
ADD_ANNOTATE_TYPE(86, DT_MAPS_ORG_CHAIN_NAME,              ui8);
ADD_ANNOTATE_TYPE(87, DT_UNUSED_1,                          ui8);
ADD_ANNOTATE_TYPE(88, DT_UNUSED_2,                          ui8);
ADD_ANNOTATE_TYPE(89, DT_UNUSED_89,                         ui8);

ADD_ANNOTATE_TYPE(90, DT_UNUSED_90,                         ui8);
ADD_ANNOTATE_TYPE(91, DT_UNUSED_91,                         ui8);
ADD_ANNOTATE_TYPE(92, DT_BANNER_ORIGINAL_PHRASE,           float);
ADD_ANNOTATE_TYPE(93, DT_BANNER_ORIGINAL_PHRASE_BID,       float);
ADD_ANNOTATE_TYPE(94, DT_BANNER_ORIGINAL_PHRASE_BID_TO_MAX_BID, ui8);

ADD_ANNOTATE_TYPE(95, DT_MOBILE_MAPS_CLICK_QFRC,           float);
ADD_ANNOTATE_TYPE(96, DT_MOBILE_MAPS_CLICK_SQFRC,          float);
ADD_ANNOTATE_TYPE(97, DT_MOBILE_MAPS_CLICK_CTR,            float);

ADD_ANNOTATE_TYPE(98, DT_VPCG_CORRECTED_CLICKS_SLP,        ui8);

ADD_ANNOTATE_TYPE(99, DT_SERP_MAPS_CLICK_QFRC,             float);
ADD_ANNOTATE_TYPE(100, DT_SERP_MAPS_CLICK_SOFRC,           float);
ADD_ANNOTATE_TYPE(101, DT_SERP_MAPS_CLICK_SQFRC,           float);

ADD_ANNOTATE_TYPE(102, DT_VIDEO_WIDE_TERM,                 float);

// Market Factors
ADD_ANNOTATE_TYPE(103, DT_MRKT_MODEL_ALIAS,                ui8);
ADD_ANNOTATE_TYPE(104, DT_MRKT_IMG_ALT_DATA,               ui8);
ADD_ANNOTATE_TYPE(105, DT_MRKT_IMG_LINK_DATA,              ui8);
ADD_ANNOTATE_TYPE(106, DT_MRKT_IMG_SHOWS_TIME,             ui8);
ADD_ANNOTATE_TYPE(107, DT_MRKT_IMG_QUERY_SHOWS_TIME,       ui8);
ADD_ANNOTATE_TYPE(108, DT_MRKT_IMG_DWELL_TIME,             ui8);
ADD_ANNOTATE_TYPE(109, DT_MRKT_IMG_QUERY_DWELL_TIME,       ui8);
ADD_ANNOTATE_TYPE(110, DT_MRKT_IMG_DOC_DWELL_TIME,         ui8);
ADD_ANNOTATE_TYPE(111, DT_MRKT_IMG_SHOWS,                  ui8);
ADD_ANNOTATE_TYPE(112, DT_MRKT_IMG_CLICKS,                 ui8);
ADD_ANNOTATE_TYPE(113, DT_MRKT_IMG_QUERY_CLICKS,           ui8);

// Yandex.Market.AdvMachine streams BEGIN
ADD_ANNOTATE_TYPE(114, DT_YA_MARKET_MANUFACTURER,          ui8);
ADD_ANNOTATE_TYPE(115, DT_YA_MARKET_CONTENTS,              ui8);
ADD_ANNOTATE_TYPE(116, DT_YA_MARKET_ALIASES,               ui8);
ADD_ANNOTATE_TYPE(117, DT_YA_MARKET_BARCODE,               ui8);
ADD_ANNOTATE_TYPE(118, DT_YA_MARKET_BOOK_PUBLISHER,        ui8);
ADD_ANNOTATE_TYPE(119, DT_YA_MARKET_BOOK_UISBN,            ui8);
ADD_ANNOTATE_TYPE(120, DT_YA_MARKET_URL,                   ui8);
ADD_ANNOTATE_TYPE(121, DT_YA_MARKET_BOOK_AUTHOR,           ui8);
ADD_ANNOTATE_TYPE(122, DT_YA_MARKET_MOBILE_MANUFACTURER,   ui8);
ADD_ANNOTATE_TYPE(123, DT_YA_MARKET_OFFER_DESCRIPTION,     float);
ADD_ANNOTATE_TYPE(124, DT_YA_MARKET_OFFER_SALES_NOTES,     float);
ADD_ANNOTATE_TYPE(125, DT_YA_MARKET_OFFER_TITLE,           float);
ADD_ANNOTATE_TYPE(126, DT_YA_MARKET_OFFER_URL,             float);
ADD_ANNOTATE_TYPE(127, DT_YA_MARKET_CATEGORY_NAME,         ui8);
// Yandex.Market.AdvMachine streams END
// Market Experimental streams
ADD_ANNOTATE_TYPE(128, DT_MRKT_UNO_EXP,                    ui8);
ADD_ANNOTATE_TYPE(129, DT_MRKT_DOS_EXP,                    ui8);
ADD_ANNOTATE_TYPE(130, DT_MRKT_TRES_EXP,                   ui8);
ADD_ANNOTATE_TYPE(131, DT_MRKT_CUATRO_EXP,                 ui8);
ADD_ANNOTATE_TYPE(132, DT_MRKT_CINCO_EXP,                  ui8);
ADD_ANNOTATE_TYPE(133, DT_MRKT_SEIS_EXP,                   ui8);
ADD_ANNOTATE_TYPE(134, DT_MRKT_SIETE_EXP,                  ui8);
ADD_ANNOTATE_TYPE(135, DT_MRKT_OCHO_EXP,                   ui8);
ADD_ANNOTATE_TYPE(136, DT_MRKT_NUEVE_EXP,                  ui8);
ADD_ANNOTATE_TYPE(137, DT_MRKT_DIEZ_EXP,                   ui8);
// Market Description stream
ADD_ANNOTATE_TYPE(138, DT_MRKT_DESCRIPTION,                ui8);
// Market order stream
ADD_ANNOTATE_TYPE(139, DT_MRKT_WAS_ORDERED,                ui8);
// Market KNN
ADD_ANNOTATE_TYPE(140, DT_MRKT_KNN_5000_9600,              ui8);
ADD_ANNOTATE_TYPE(141, DT_MRKT_KNN_9600_10000,             ui8);
ADD_ANNOTATE_TYPE(142, DT_MRKT_KNN_10000_11000,            ui8);
ADD_ANNOTATE_TYPE(143, DT_MRKT_KNN_11000_20000,            ui8);
// Market user session stream
ADD_ANNOTATE_TYPE(144, DT_YA_MARKET_SPLIT_DT,              ui8);
ADD_ANNOTATE_TYPE(145, DT_YA_MARKET_ONE_CLICK,             ui8);
ADD_ANNOTATE_TYPE(146, DT_YA_MARKET_LONG_CLICK,            ui8);
ADD_ANNOTATE_TYPE(147, DT_YA_MARKET_LONG_CLICK_120_SQRT,   ui8);
ADD_ANNOTATE_TYPE(148, DT_YA_MARKET_SIMPLE_CLICK,          ui8);
ADD_ANNOTATE_TYPE(149, DT_YA_MARKET_LONG_CLICK_MOBILE,     ui8);
ADD_ANNOTATE_TYPE(150, DT_YA_MARKET_FIRST_LAST_CLICK_MOBILE, ui8);

ADD_ANNOTATE_TYPE(151, DT_VIDEOHUB_TERM,                   float);
ADD_ANNOTATE_TYPE(152, DT_MRKT_RECIPE,                     ui8);
ADD_ANNOTATE_TYPE(153, DT_BLUE_MRKT_DESCRIPTION,           ui8);
ADD_ANNOTATE_TYPE(154, DT_BLUE_MRKT_TITLE,                 ui8);

ADD_ANNOTATE_TYPE(155, DT_LOCATED_AT_ORG_NAME,             ui8);
ADD_ANNOTATE_TYPE(156, DT_FACULTY_NAME,                    ui8);

ADD_ANNOTATE_TYPE(157, DT_SERNAMECRC,                      float);
ADD_ANNOTATE_TYPE(158, DT_SERNAMECRC_PLUS_SEASON,          float);

ADD_ANNOTATE_TYPE(159, DT_HOSPITAL_NAME,                   ui8);
ADD_ANNOTATE_TYPE(160, DT_ORG_SUMMARY,                     ui8);

ADD_ANNOTATE_TYPE(161, DT_BLUE_MRKT_MODEL_TITLE,           ui8);
ADD_ANNOTATE_TYPE(162, DT_BLUE_MRKT_MODEL_ALIAS,           ui8);

ADD_ANNOTATE_TYPE(163, DT_MRKT_TITLE,                      ui8);

ADD_ANNOTATE_TYPE(164, DT_ADV_MACHINE_1, ui8);
ADD_ANNOTATE_TYPE(165, DT_ADV_MACHINE_2, ui8);
ADD_ANNOTATE_TYPE(166, DT_ADV_MACHINE_3, ui8);
ADD_ANNOTATE_TYPE(167, DT_ADV_MACHINE_4, ui8);
ADD_ANNOTATE_TYPE(168, DT_ADV_MACHINE_5, ui8);
ADD_ANNOTATE_TYPE(169, DT_ADV_MACHINE_6, ui8);
ADD_ANNOTATE_TYPE(170, DT_ADV_MACHINE_7, ui8);
ADD_ANNOTATE_TYPE(171, DT_ADV_MACHINE_8, ui8);

ADD_ANNOTATE_TYPE(172, DT_MRKT_MARKETING_DESCR, ui8);
ADD_ANNOTATE_TYPE(173, DT_BLUE_MRKT_MARKETING_DESCR, ui8);
ADD_ANNOTATE_TYPE(174, DT_BLUE_MRKT_MICRO_MODEL_DESCR, ui8);

ADD_ANNOTATE_TYPE(175, DT_ALICE_MUSIC_STREAMS_TRACK_TITLE, ui8);
ADD_ANNOTATE_TYPE(176, DT_ALICE_MUSIC_STREAMS_ARTIST_NAME, ui8);
ADD_ANNOTATE_TYPE(177, DT_ALICE_MUSIC_STREAMS_ALBUM_TITLE, ui8);
ADD_ANNOTATE_TYPE(178, DT_ALICE_MUSIC_STREAMS_TRACK_ARTIST_NAMES, ui8);
ADD_ANNOTATE_TYPE(179, DT_ALICE_MUSIC_STREAMS_TRACK_ALBUM_TITLE, ui8);
ADD_ANNOTATE_TYPE(180, DT_ALICE_MUSIC_STREAMS_TRACK_LYRICS, ui8);

ADD_ANNOTATE_TYPE(181, DT_VIDEO_SPEECH_TO_TEXT, ui8);

ADD_ANNOTATE_TYPE(182, DT_MUSIC_STREAMS_FIRST_CLICK, ui8);
ADD_ANNOTATE_TYPE(183, DT_MUSIC_STREAMS_LAST_CLICK, ui8);
ADD_ANNOTATE_TYPE(184, DT_MUSIC_STREAMS_LONG_CLICK, ui8);
ADD_ANNOTATE_TYPE(185, DT_MUSIC_STREAMS_SINGLE_CLICK, ui8);

ADD_ANNOTATE_TYPE(186, DT_MRKT_MICRO_MODEL_DESCR, ui8);

ADD_ANNOTATE_TYPE(187, DT_VIDEO_OCR, ui8);

ADD_ANNOTATE_TYPE(188, DT_RC_SEARCH_CLICKS_D30T5, ui8); // USERFEAT-1526

ADD_ANNOTATE_TYPE(189, DT_VIDEO_CLICK_SIM, ui8);

ADD_ANNOTATE_TYPE(190, DT_MRKT_MSKU_OFFER_TITLE, ui8);
ADD_ANNOTATE_TYPE(191, DT_MRKT_MSKU_OFFER_SEARCH_TEXT, ui8);

ADD_ANNOTATE_TYPE(192, DT_MRKT_METADOC_BQPR, ui8);
ADD_ANNOTATE_TYPE(193, DT_MRKT_METADOC_FIRST_CLICK_DT_XF, ui8);
ADD_ANNOTATE_TYPE(194, DT_MRKT_METADOC_LONG_CLICK, ui8);
ADD_ANNOTATE_TYPE(195, DT_MRKT_METADOC_LONG_CLICK_SP, ui8);
ADD_ANNOTATE_TYPE(196, DT_MRKT_METADOC_ONE_CLICK, ui8);
ADD_ANNOTATE_TYPE(197, DT_MRKT_METADOC_SIMPLE_CLICK, ui8);
ADD_ANNOTATE_TYPE(198, DT_MRKT_METADOC_SPLIT_DT, ui8);
ADD_ANNOTATE_TYPE(199, DT_MRKT_METADOC_BQPR_SAMPLE, ui8);
ADD_ANNOTATE_TYPE(200, DT_MRKT_METADOC_NHOP, ui16);

ADD_ANNOTATE_TYPE(201, DT_KP_GENRE, ui8);
ADD_ANNOTATE_TYPE(202, DT_KP_CAST, ui8);
ADD_ANNOTATE_TYPE(203, DT_KP_CHARACTER, ui8);
ADD_ANNOTATE_TYPE(204, DT_KP_KEYWORD, float);
ADD_ANNOTATE_TYPE(205, DT_KP_AWARD, ui8);
ADD_ANNOTATE_TYPE(206, DT_KP_TAG, float);
ADD_ANNOTATE_TYPE(207, DT_KP_I2I_SIMILARITY, ui8);
ADD_ANNOTATE_TYPE(208, DT_KP_HANDMADE_SIMILARITY, ui8);

ADD_ANNOTATE_TYPE(209, DT_MRKT_CPA_QUERY, ui8);

ADD_ANNOTATE_TYPE(210, DT_KP_PERSON_CHARACTER, ui8);
ADD_ANNOTATE_TYPE(211, DT_PRS_POSITION, float);

ADD_ANNOTATE_TYPE(212, DT_VIDEO_HASHTAGS, ui8);
ADD_ANNOTATE_TYPE(213, DT_VIDEO_TRANSLATED_TITLE, ui8);
ADD_ANNOTATE_TYPE(214, DT_VIDEO_TRANSLATED_TEXT, ui8);

ADD_ANNOTATE_TYPE(215, DT_MRKT_VENDOR_NAME, ui8);
ADD_ANNOTATE_TYPE(216, DT_MRKT_SHOP_NAME, ui8);
ADD_ANNOTATE_TYPE(217, DT_MRKT_SHOP_CATEGORIES, ui8);
ADD_ANNOTATE_TYPE(218, DT_MRKT_NID_NAMES, ui8);

    // LinkAnn streams
// int32
constexpr uint8_t DT_LINKANN_UINT32_SOURCE_LAST_ACCESS = 8;
constexpr uint8_t DT_UNUSED_9 = 9;
constexpr uint8_t DT_UNUSED_10 = 10;

//bool
constexpr uint8_t DT_LINKANN_BOOL_IS_INTERNAL = 11;
constexpr uint8_t DT_LINKANN_BOOL_INDICATOR = 12;

constexpr uint8_t DT_LINKANN_FLOAT_MULTIPLICITY = 14;

COUNTER_INC;
constexpr EDataType DT_NUM_TYPES = static_cast<EDataType>(COUNTER_READ);

typedef TBitMap<DT_NUM_TYPES> TTypesMask;
THashMap<size_t, size_t> GetAvailableTypes();

class TWebQuorumProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_NHOP:
            case DT_WT:
            case DT_BEAST_Q:
            case DT_CORRECTED_CTR:
            case DT_META_TAG_SENTENCE:
            case DT_RC_SEARCH_CLICKS_D30T5:
                return true;
            default:
                return false;
        }
    }
};

class TWebFactorProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_SAMPLE_PERIOD:
            case DT_SPLIT_DT:
            case DT_ONE_CLICK:
            case DT_LONG_CLICK:
            case DT_LONG_CLICK_SP:
            case DT_SIMPLE_CLICK:
            case DT_BQPR:
            case DT_YABAR_TIME:
            case DT_YABAR_VISITS:
            case DT_RANDOM_LOG_DBM35:
            case DT_CORRECTED_CTR_XFACTOR:
            case DT_EXP:
            case DT_POPULAR_SE_FRC_BROWSER:
            case DT_DOUBLE_FRC:
            case DT_BQPR_SAMPLE:
            case DT_ONE_CLICK_FRC_XF_SP:
            case DT_QUERY_DWELL_TIME:
            case DT_CORRECTED_CTR_LONG_PERIOD:
            case DT_NHOP_SUM_DWELL_TIME:
            case DT_FIRST_CLICK_DT_XF:
            case DT_LONG_CLICK_MOBILE:
            case DT_FIRST_LAST_CLICK_MOBILE:
            case DT_AVG_DT_WEIGHTED_BY_RANK_MOBILE:
            case DT_VPCG_CORRECTED_CLICKS_SLP:
            case DT_VIDEO_PCTR_NEW:
            case DT_ALICE_MUSIC_STREAMS_TRACK_TITLE:
            case DT_ALICE_MUSIC_STREAMS_ARTIST_NAME:
            case DT_ALICE_MUSIC_STREAMS_ALBUM_TITLE:
            case DT_ALICE_MUSIC_STREAMS_TRACK_ARTIST_NAMES:
            case DT_ALICE_MUSIC_STREAMS_TRACK_ALBUM_TITLE:
            case DT_ALICE_MUSIC_STREAMS_TRACK_LYRICS:
                return true;
            default:
                return false;
        }
    }
};

class TSaaSQuorumProfile : public IFeaturesProfile {
private:
    TWebQuorumProfile WebProfile;

public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_WT:
            case DT_BEAST_Q:
                return false;
            case DT_MUSIC_STREAMS_FIRST_CLICK:
            case DT_MUSIC_STREAMS_LAST_CLICK:
            case DT_MUSIC_STREAMS_LONG_CLICK:
            case DT_MUSIC_STREAMS_SINGLE_CLICK:
            case DT_KP_GENRE:
            case DT_KP_CAST:
            case DT_KP_CHARACTER:
            case DT_KP_KEYWORD:
            case DT_KP_AWARD:
            case DT_KP_TAG:
            case DT_KP_I2I_SIMILARITY:
            case DT_KP_HANDMADE_SIMILARITY:
            case DT_KP_PERSON_CHARACTER:
                return true;
            default:
                return WebProfile.IsEssential(type);
        }
    }
};

class TSaaSFactorProfile : public IFeaturesProfile {
private:
    TWebFactorProfile WebProfile;

public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_RANDOM_LOG_DBM35:
            case DT_NHOP_SUM_DWELL_TIME:
                return false;
            case DT_MUSIC_STREAMS_FIRST_CLICK:
            case DT_MUSIC_STREAMS_LAST_CLICK:
            case DT_MUSIC_STREAMS_LONG_CLICK:
            case DT_MUSIC_STREAMS_SINGLE_CLICK:
            case DT_KP_GENRE:
            case DT_KP_CAST:
            case DT_KP_CHARACTER:
            case DT_KP_KEYWORD:
            case DT_KP_AWARD:
            case DT_KP_TAG:
            case DT_KP_I2I_SIMILARITY:
            case DT_KP_HANDMADE_SIMILARITY:
            case DT_KP_PERSON_CHARACTER:
                return true;
            default:
                return WebProfile.IsEssential(type);
        }
    }
};

class TVideoQuorumProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_CORRECTED_CLICKS:
            case DT_NHOP_VIDEO:
                return true;
            default:
                return false;
        }
    }
};

class TVideoFactorProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_VIDEO_QUSM:
            case DT_RANDOM_LOG_DBM35:
            case DT_VIDEO_MAX_CLICKS_PERCENT:
            case DT_VIDEO_PCTR_NEW:
            case DT_VIDEO_PLAYER_VIEW_TIME_MAX_LOKI:
            case DT_CORRECTED_CTR_XFACTOR:
            case DT_ONE_CLICK_FRC_XF_SP:
            case DT_LONG_CLICK_SP:
            case DT_SIMPLE_CLICK:
            case DT_BQPR_SAMPLE:
            case DT_VIDEO_SPEECH_TO_TEXT:
            case DT_VIDEO_OCR:
            case DT_VIDEO_CLICK_SIM:
            case DT_VIDEO_HASHTAGS:
            case DT_VIDEO_TRANSLATED_TITLE:
            case DT_VIDEO_TRANSLATED_TEXT:
                return true;
            default:
                return false;
        }
    }
};

class TVideoTermProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_VIDEO_RELATED_TERM:
            case DT_VIDEO_WIDE_TERM:
            case DT_VIDEOHUB_TERM:
                return true;
            default:
                return false;
        }
    }
};

class TBannerBaseProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_BANNER_PHRASE_BID:
            case DT_BANNER_PHRASE_HISTORY_CPM:
            case DT_BANNER_PHRASE_HISTORY_CPC:
            case DT_BANNER_PHRASE_CTR:
            case DT_BANNER_PHRASE_FRC:
            case DT_BANNER_PHRASE_XF_CTR:
            case DT_BANNER_TITLE:
            case DT_BANNER_TEXT:
            case DT_BANNER_COMM_QUERIES_CTR:
            case DT_BANNER_COMM_QUERIES_FRC:
            case DT_BANNER_COMM_QUERIES_XF_CTR:
            case DT_CORRECTED_CTR:
            case DT_BQPR:
            case DT_RANDOM_LOG_DBM35:
            case DT_BANNER_PHRASE_RSYA_CTR:
            case DT_BANNER_PHRASE_RSYA_FRC:
            case DT_BANNER_SPY_LOG_TITLE:
            case DT_LANDING_PAGE_TITLE:
            case DT_LANDING_PAGE_TEXT:
            case DT_BANNER_PHRASE_GUARANTEE_BID:
            case DT_BANNER_PHRASE_GUARANTEE_CTR:
            case DT_BANNER_PHRASE_GUARANTEE_HISTORY_CPC:
            case DT_BANNER_COMM_QUERIES_GUARANTEE_CTR:
            case DT_BANNER_COMM_QUERIES_BID:
            case DT_BANNER_COMM_QUERIES_HISTORY_CPC:
            case DT_BANNER_RSYA_QUERY_BID:
            case DT_BANNER_RSYA_QUERY_CTR:
            case DT_BANNER_RSYA_QUERY_FRC:
            case DT_BANNER_RSYA_QUERY_HISTORY_CPC:
            case DT_BANNER_QUERY_CLICKS_FR:
            case DT_BANNER_RSYA_QUERY_CLICKS_FR:
            case DT_BANNER_QUERY_COST_FR:
            case DT_BANNER_RSYA_QUERY_COST_FR:
            case DT_UNUSED_1:
            case DT_UNUSED_2:
            case DT_BANNER_ORIGINAL_PHRASE:
            case DT_BANNER_ORIGINAL_PHRASE_BID:
            case DT_BANNER_ORIGINAL_PHRASE_BID_TO_MAX_BID:
            case DT_YA_MARKET_MANUFACTURER:
            case DT_YA_MARKET_CATEGORY_NAME:
            case DT_YA_MARKET_CONTENTS:
            case DT_YA_MARKET_ALIASES:
            case DT_YA_MARKET_BARCODE:
            case DT_YA_MARKET_BOOK_PUBLISHER:
            case DT_YA_MARKET_BOOK_UISBN:
            case DT_YA_MARKET_URL:
            case DT_YA_MARKET_BOOK_AUTHOR:
            case DT_YA_MARKET_MOBILE_MANUFACTURER:
            case DT_YA_MARKET_OFFER_DESCRIPTION:
            case DT_YA_MARKET_OFFER_SALES_NOTES:
            case DT_YA_MARKET_OFFER_TITLE:
            case DT_YA_MARKET_OFFER_URL:
            case DT_ADV_MACHINE_1:
            case DT_ADV_MACHINE_2:
            case DT_ADV_MACHINE_3:
            case DT_ADV_MACHINE_4:
            case DT_ADV_MACHINE_5:
            case DT_ADV_MACHINE_6:
            case DT_ADV_MACHINE_7:
            case DT_ADV_MACHINE_8:
                return true;
            default:
                return false;
        }
    }
};

class TBannerExtProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_BANNER_PHRASE_CLICKS:
            case DT_BANNER_PHRASE_RSYA_CLICKS:
            case DT_BANNER_RSYA_PHRASE_BID:
            case DT_BANNER_PHRASE_RSYA_HISTORY_CPC:
            case DT_BANNER_PHRASE_GUARANTEE_CLICKS:
            case DT_BANNER_COMM_QUERIES_CLICKS:
            case DT_BANNER_COMM_QUERIES_GUARANTEE_CLICKS:
                return true;
            default:
                return false;
        }
    }
};


class TBannerProfile : public TComboProfile {
public:
    TBannerProfile(bool extended = false)
        : TComboProfile({
            new TWebFactorProfile(),
            new TBannerBaseProfile()
        })
    {
        if (extended) {
            AddProfile(new TBannerExtProfile());
        }
    }
};


class TBannerMinusProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            case DT_BANNER_MINUS:
                return true;
            default:
                return false;
        }
    }
};

class TBannerBothProfile : public TComboProfile {
public:
    TBannerBothProfile(bool extended = false)
        : TComboProfile({
            new TBannerProfile(extended),
            new TBannerMinusProfile()
        })
    {
    }
};

class TOmmFactorProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
        case DT_CORRECTED_CTR_XFACTOR:
        case DT_CORRECTED_CTR:
        case DT_BQPR_SAMPLE:
        case DT_POPULAR_SE_FRC_BROWSER:
        case DT_YABAR_VISITS:

        case DT_BANNER_TITLE:
        case DT_BANNER_TEXT:
        case DT_LANDING_PAGE_TITLE:

        case DT_YA_MARKET_ALIASES:
        case DT_YA_MARKET_URL:
        case DT_YA_MARKET_OFFER_DESCRIPTION:
        case DT_YA_MARKET_OFFER_TITLE:
        case DT_YA_MARKET_OFFER_URL:
        case DT_YA_MARKET_MANUFACTURER:
        case DT_YA_MARKET_CATEGORY_NAME:

        // Marker user session streams
        case DT_YA_MARKET_SPLIT_DT:
        case DT_YA_MARKET_ONE_CLICK:
        case DT_YA_MARKET_LONG_CLICK:
        case DT_YA_MARKET_LONG_CLICK_120_SQRT:
        case DT_YA_MARKET_SIMPLE_CLICK:
        case DT_YA_MARKET_LONG_CLICK_MOBILE:
        case DT_YA_MARKET_FIRST_LAST_CLICK_MOBILE:
            return true;
        default:
            return false;
        }
    }
};

class TMarketFactorProfile : public IFeaturesProfile {
public:
    bool IsEssential(size_t type) const override {
        switch (type) {
            // Web streams
            case DT_BQPR:
            case DT_FIRST_CLICK_DT_XF:
            case DT_AVG_DT_WEIGHTED_BY_RANK_MOBILE:
            case DT_LONG_CLICK:
            case DT_LONG_CLICK_SP:
            case DT_ONE_CLICK:
            case DT_SIMPLE_CLICK:
            case DT_SPLIT_DT:
            case DT_BQPR_SAMPLE:
            case DT_NHOP:
            case DT_MRKT_MODEL_ALIAS:
            // Image streams
            case DT_MRKT_IMG_ALT_DATA:
            case DT_MRKT_IMG_LINK_DATA:
            case DT_MRKT_IMG_SHOWS_TIME:
            case DT_MRKT_IMG_QUERY_SHOWS_TIME:
            case DT_MRKT_IMG_DWELL_TIME:
            case DT_MRKT_IMG_QUERY_DWELL_TIME:
            case DT_MRKT_IMG_DOC_DWELL_TIME:
            case DT_MRKT_IMG_SHOWS:
            case DT_MRKT_IMG_CLICKS:
            case DT_MRKT_IMG_QUERY_CLICKS:
            // Exp streams
            case DT_MRKT_UNO_EXP:
            case DT_MRKT_DOS_EXP:
            case DT_MRKT_TRES_EXP:
            case DT_MRKT_CUATRO_EXP:
            case DT_MRKT_CINCO_EXP:
            case DT_MRKT_SEIS_EXP:
            case DT_MRKT_SIETE_EXP:
            case DT_MRKT_OCHO_EXP:
            case DT_MRKT_NUEVE_EXP:
            case DT_MRKT_DIEZ_EXP:
            case DT_MRKT_DESCRIPTION:
            // metadoc streams:
            case DT_MRKT_METADOC_BQPR:
            case DT_MRKT_METADOC_FIRST_CLICK_DT_XF:
            case DT_MRKT_METADOC_LONG_CLICK:
            case DT_MRKT_METADOC_LONG_CLICK_SP:
            case DT_MRKT_METADOC_ONE_CLICK:
            case DT_MRKT_METADOC_SIMPLE_CLICK:
            case DT_MRKT_METADOC_SPLIT_DT:
            case DT_MRKT_METADOC_BQPR_SAMPLE:
            case DT_MRKT_METADOC_NHOP:
            // Advertisement streams
            case DT_YA_MARKET_ALIASES:
            case DT_YA_MARKET_URL:
            case DT_YA_MARKET_OFFER_DESCRIPTION:
            case DT_YA_MARKET_OFFER_TITLE:
            case DT_YA_MARKET_OFFER_URL:
            case DT_YA_MARKET_MANUFACTURER:
            case DT_YA_MARKET_CATEGORY_NAME:
            case DT_YA_MARKET_OFFER_SALES_NOTES:
            // Market Streams
            case DT_MRKT_WAS_ORDERED:
            case DT_MRKT_RECIPE:
            case DT_MRKT_TITLE:
            case DT_MRKT_MARKETING_DESCR:
            case DT_BLUE_MRKT_MARKETING_DESCR:
            case DT_BLUE_MRKT_MICRO_MODEL_DESCR:
            case DT_MRKT_MICRO_MODEL_DESCR:
            case DT_MRKT_CPA_QUERY:
            case DT_MRKT_VENDOR_NAME:
            case DT_MRKT_SHOP_NAME:
            case DT_MRKT_SHOP_CATEGORIES:
            case DT_MRKT_NID_NAMES:
            // KNN streams
            case DT_MRKT_KNN_5000_9600:
            case DT_MRKT_KNN_9600_10000:
            case DT_MRKT_KNN_10000_11000:
            case DT_MRKT_KNN_11000_20000:
            // Click machine factors
            case DT_YA_MARKET_SPLIT_DT:
            case DT_YA_MARKET_ONE_CLICK:
            case DT_YA_MARKET_LONG_CLICK:
            case DT_YA_MARKET_LONG_CLICK_120_SQRT:
            case DT_YA_MARKET_SIMPLE_CLICK:
            case DT_YA_MARKET_FIRST_LAST_CLICK_MOBILE:
            // Blue Market Streams
            case DT_BLUE_MRKT_TITLE:
            case DT_BLUE_MRKT_DESCRIPTION:
            case DT_BLUE_MRKT_MODEL_TITLE:
            case DT_BLUE_MRKT_MODEL_ALIAS:
            // Msku streams
            case DT_MRKT_MSKU_OFFER_TITLE:
            case DT_MRKT_MSKU_OFFER_SEARCH_TEXT:
                return true;
            default:
                return false;
        }
    }
};

// Keep only fields that are essential.
// @return true if there are at least one essential field.
bool FilterEssentialFeatures(const IFeaturesProfile& profile, const ::NIndexAnn::TIndexAnnSiteData& src, ::NIndexAnn::TIndexAnnSiteData& dst);

bool FilterEssentialFeatures(const IFeaturesProfile& profile, const ::NIndexAnn::TAnnotationRec& src, ::NIndexAnn::TAnnotationRec& dst);

void EnumerateFeatures(const ::NIndexAnn::TAnnotationRec& src, TVector<::NIndexAnn::EDataType>& features);

#undef COUNTER_READ
#undef COUNTER_INC
#undef ADD_ANNOTATE_TYPE

} // namespace NIndexAnn
