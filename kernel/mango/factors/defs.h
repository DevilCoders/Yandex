#pragma once
// unique tag to fix pragma once gcc glueing: ./kernel/mango/factors/defs.h

#include <kernel/mango/common/constraints.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NMango
{

    enum TQuoteFactorsLine
    {
        LINKS_COUNT = 0,
        RESOURCE_COUNT,
        MAX_AUTHORITY,
        AUTHOR_COUNT,
        LANGS_COUNT,
        DIVERSITY,
        AUTHOR_AUTHORITY_SUM,
        QUOTE_AUTHORITY_SUM,
        AUTHOR_AUTHORITY_Q50,
        AUTHOR_AUTHORITY_Q75,
        AUTHOR_AUTHORITY_Q90,
        AUTHOR_AUTHORITY_AVG75,
        SPAM_COUNT,

        SPAM_PROBABILITY,
        AVERAGE_QUOTE_AUTHORITY,
        AVERAGE_AUTHOR_AUTHORITY,
        UNIQUE_AUTHORS_RATIO,
        UNIQUE_AUTHORS_RATIO_BY_AUTHORITY,

        TQuoteFactorsLine_Last
    };

    enum TOtherStaticFactorsLine
    {
        IS_MORDA,
        CREATION_TIME,
        TWEET_COUNT,
        LIKE_AUTHORITY_SUM,
        LIKE_COUNT,
        AUTH_PREDICT_KOEF,
        AUTHORITY_STATRANK,

        TOtherStaticFactorsLine_Last
    };

    enum TRelevanceFactorsLine
    {
        COS_TR = 0,
        MANGO_TR,
        PHRASE_TR,
        TITLE_TR,

        TRelevanceFactorsLine_Last
    };

    enum TDocRelevanceFactorsLine
    {
        POS_RELEVANCE = 0,
        POS_RELEVANCE_NORM_BY_DOC,
        POS_RELEVANCE_NORM_BY_QUERY,
        FLAT_RELEVANCE,

        TDocRelevanceFactorsLine_Last
    };

    enum TOtherDynamicFactorsLine
    {
        TOPIC_SHARE = 0,
        AUTHOR_TOPIC_SHARE,
        UNWEIGHTED_TOPIC_SHARE,
        UNWEIGHTED_AUTHOR_TOPIC_SHARE,
        DECAYED_AUTHOR_AUTHORITY_SUM,
        DECAYED_QUOTE_AUTHORITY_SUM,
        FORCED_LINKS_COUNT,
        MEGA_FORCED_LINKS_COUNT,
        TITLE_LINKS_COUNT,
        NORMALIZED_LINKS_COUNT,

        TOtherDynamicFactorsLine_Last
    };

    enum TFinalDynamicFactorsLine
    {
        SIMPLE_RELEVANCE_SUM = 0,
        SIMPLE_RELEVANCE_AVG,
        SIMPLE_RELEVANCE_AT,
        SIMPLE_RELEVANCE_MNG_SUM,
        SIMPLE_RELEVANCE_MNG_AVG,
        SIMPLE_RELEVANCE_MNG_AT,
        SIMPLE_INTERESTINGNESS,
        SIMPLE_PESSIMISATION,
        SIMPLE_TRASHLESS,
        SIMPLE_STATIC_INTERESTINGNESS,
        SIMPLE_STATIC_TRASHLESS,
        SIMPLE_HOST_INTERESTINGNESS,
        SIMPLE_HOST_TRASHLESS,
        SIMPLE_FORMULA_1,

        TFinalDynamicFactorsLine_Last
    };


    enum TWindow
    {
        WINDOW_0 = 0,
        WINDOW_1,
        WINDOW_2,
        WINDOW_3,

        NO_WINDOW
    };

#define REPEAT_WINDOWS(MACRO) \
    MACRO(0) \
    MACRO(1) \
    MACRO(2) \
    MACRO(3)

#define ENUM_WINDOW_GROUPS_TO_MACRO(MACRO_START, MACRO_END, GROUP_TERMINATOR, I) \
    MACRO_START Y_CAT(GROUP_W##I, _QUOTE_REL) MACRO_END GROUP_TERMINATOR \
    MACRO_START Y_CAT(GROUP_W##I, _RELEVANCE_SUM) MACRO_END GROUP_TERMINATOR \
    MACRO_START Y_CAT(GROUP_W##I, _RELEVANCE_AVG) MACRO_END GROUP_TERMINATOR \
    MACRO_START Y_CAT(GROUP_W##I, _RELEVANCE_AT) MACRO_END GROUP_TERMINATOR \
    MACRO_START Y_CAT(GROUP_W##I, _OTHER) MACRO_END GROUP_TERMINATOR \
    MACRO_START Y_CAT(GROUP_W##I, _FINAL) MACRO_END GROUP_TERMINATOR

#define MANGO_COMMA ,
#define ENUM_WINDOW_GROUPS(I) \
    ENUM_WINDOW_GROUPS_TO_MACRO(, , MANGO_COMMA, I)

    enum TFactorGroups
    {
        GROUP_FIRST_DUMMY = -1,

        GROUP_QUOTE_STATIC = 0,
        GROUP_QUOTE_HOST,
        GROUP_OTHER_STATIC,

        REPEAT_WINDOWS(ENUM_WINDOW_GROUPS)

        GROUP_DOC_RELEVANCE,

        // keep this in sync, please
        GROUP_LAST = GROUP_DOC_RELEVANCE
    };

    template <TFactorGroups gr>
    struct TGroupTraits {};

    template <>
    struct TGroupTraits<GROUP_FIRST_DUMMY> { enum { EndOffset = 1 }; };

#define DECLARE_GROUP(group, T) \
    template <> \
    struct TGroupTraits<group> \
    { \
        typedef T TLineType; \
        enum { \
            EndOffset = TGroupTraits<static_cast<TFactorGroups>(group - 1)>::EndOffset + T##_Last, \
            Offset    = TGroupTraits<static_cast<TFactorGroups>(group - 1)>::EndOffset, \
            Size      = T##_Last \
        }; \
        static int GetGlobalId(TLineType id) \
        { \
            return Offset + id; \
        } \
    };

#define DECLARE_WINDOW_GROUPS(I) \
    DECLARE_GROUP(Y_CAT(GROUP_W##I, _QUOTE_REL), TQuoteFactorsLine) \
    DECLARE_GROUP(Y_CAT(GROUP_W##I, _RELEVANCE_SUM), TRelevanceFactorsLine) \
    DECLARE_GROUP(Y_CAT(GROUP_W##I, _RELEVANCE_AVG), TRelevanceFactorsLine) \
    DECLARE_GROUP(Y_CAT(GROUP_W##I, _RELEVANCE_AT), TRelevanceFactorsLine) \
    DECLARE_GROUP(Y_CAT(GROUP_W##I, _OTHER), TOtherDynamicFactorsLine) \
    DECLARE_GROUP(Y_CAT(GROUP_W##I, _FINAL), TFinalDynamicFactorsLine)

    DECLARE_GROUP(GROUP_QUOTE_STATIC, TQuoteFactorsLine)
    DECLARE_GROUP(GROUP_QUOTE_HOST, TQuoteFactorsLine)
    DECLARE_GROUP(GROUP_OTHER_STATIC, TOtherStaticFactorsLine)

    REPEAT_WINDOWS(DECLARE_WINDOW_GROUPS)

    DECLARE_GROUP(GROUP_DOC_RELEVANCE, TDocRelevanceFactorsLine)

    template <TWindow wnd>
    struct TWindowTraits {};


#define DECLARE_WINDOW(I) \
    template <> \
    struct TWindowTraits<WINDOW_##I> \
    { \
        enum { \
            QuoteRel = Y_CAT(GROUP_W##I, _QUOTE_REL), \
            RelSum   = Y_CAT(GROUP_W##I, _RELEVANCE_SUM), \
            RelAvg   = Y_CAT(GROUP_W##I, _RELEVANCE_AVG), \
            RelAt    = Y_CAT(GROUP_W##I, _RELEVANCE_AT), \
            Other    = Y_CAT(GROUP_W##I, _OTHER), \
            Final    = Y_CAT(GROUP_W##I, _FINAL) \
        }; \
    };

    REPEAT_WINDOWS(DECLARE_WINDOW)


    struct TWindowDescriptor
    {
        time_t Period;
        TString Marker;
        TFinalDynamicFactorsLine Factor;
        TWindow WindowType;
        float Fading;

        TWindowDescriptor(time_t period, const TString &marker, TFinalDynamicFactorsLine factor, TWindow windowType, float fading)
            : Period(period)
            , Marker(marker)
            , Factor(factor)
            , WindowType(windowType)
            , Fading(fading)
        {}
    };

    const TVector<TWindowDescriptor> WindowDescriptors = {
        TWindowDescriptor(DEFAULT_DECAY_PERIOD, "1Y",  SIMPLE_FORMULA_1, WINDOW_0, 0.5f),
        TWindowDescriptor(12 * 3600,            "12H", SIMPLE_FORMULA_1, WINDOW_1, 0.5f),
        TWindowDescriptor(1 * 24 * 3600,        "1D",  SIMPLE_FORMULA_1, WINDOW_2, 0.5f),
        TWindowDescriptor(3 * 24 * 3600,        "3D",  SIMPLE_FORMULA_1, WINDOW_3, 0.5f)};
}

