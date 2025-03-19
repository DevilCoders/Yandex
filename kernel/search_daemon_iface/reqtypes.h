#pragma once

enum REQUEST_PROCESSING_FLAG {
    RPF_DEFAULT                         = 0x00000000,
    RPF_DOCUMENT_PHRASE_LEVEL           = 0x00000004,
    RPF_RELEVANCE_EXTEND                = 0x00000008,

    // Enable features that are not listed at http://help.yandex.ru/search/query-language/crib-sheet.xml
    RPF_ENABLE_EXTENDED_SYNTAX          = 0x00000010,
    RPF_D449_ON_FASTRANK                = 0x00000020,
    RPF_SENTENCE_PHRASE_LEVEL           = 0x00000040,
    RPF_EXACT_PHRASE_LEVEL              = 0x00000080,
    RPF_CLEAN_REQUESTS_ONLY             = 0x00010000, // throw exception on requests that used recovery
    RPF_PRUNING                         = 0x00020000,
    RPF_USE_TOKEN_CLASSIFIER            = 0x00040000,
    RPF_REMOVE_DOCANDS                  = 0x00080000, // removes "&&" operators from request tree
    RPF_DO_QUORUM                       = 0x00100000,
    RPF_SPLIT_EMAILS                    = 0x00200000,
    RPF_FASTRANK                        = 0x00400000,
    RPF_JUST_FASTRANK                   = 0x00800000,
    RPF_TRIM_EXTRA_TOKENS               = 0x01000000,
    RPF_NO_FREQ_BAN                     = 0x02000000,
    PRF_WILDCARDS                       = 0x04000000,
    RPF_NO_AUTO_ACCEPT                  = 0x08000000,
    RPF_SMART_DYNAMIC_QUORUM            = 0x10000000,
    RPF_DONT_FIX_SYNTAX                 = 0x20000000,
    RPF_DONT_READJUST_JOINS             = 0x40000000, // TODO: elric@ drop
    RPF_NUMBERS_STYLE_NO_ZEROS          = 0x80000000,
};

// Warning: Don't change order in this enumeration, because of usage
// of it's numbers in loadlog.
// Note: RT_Other doesn't mean a some special request type and used by
// unistat signals to join uninteresting signals together
enum ERequestType {
    RT_Unknown      /* "unknown" */,
    RT_Search       /* "search" */,
    RT_Fetch        /* "fetch" */,
    RT_Factors      /* "factors" */,
    RT_Info         /* "info" */,
    RT_Reask        /* "reask" */,
    RT_LongSearch   /* "longsearch" */,
    RT_DbgSearch    /* "dbgsearch" */,
    RT_PreSearch    /* "presearch" */,
    RT_BadRequest   /* "badrequest" */,
    RT_Admin        /* "admin" */,
    RT_Supermind    /* "supermind" */,
    RT_Tass         /* "tass" */,
    RT_Metric       /* "metric" */,
    RT_Static       /* "static" */,
    RT_Hilite       /* "hilite" */,
    RT_FetchDocData /* "fetchdocdata" */,
    RT_Ping         /* "ping" */,
    RT_FetchAttrs   /* "fetchattrs" */,
    RT_Other        /* "other" */
};
