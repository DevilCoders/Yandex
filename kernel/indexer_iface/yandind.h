#pragma once

#include <sys/types.h>
#include <cstring>

#include <library/cpp/html/face/parstypes.h> // ???? temp
#include <library/cpp/langmask/langmask.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YXOK    0
#define YXFALSE 1

typedef void *YX_LOGOBJ;

typedef
enum LOG_LEVEL
{
    LL_NONE        =   0,
    LL_CONFIG      =   1,
    LL_MORECONFIG  =   2,
    LL_CRITERROR   =   4,
    LL_ERROR       =   8,
    LL_WARNING     =  16,
    LL_MOREWARNING =  32,
    LL_INFO        =  64,
    LL_MOREINFO    = 128,
    LL_DEBUG       = 256,
    LL_MOREDEBUG   = 512
} LOG_LEVEL;

typedef void (*YX_LOGNOTIFY)(YX_LOGOBJ Obj, LOG_LEVEL level, const char* format, ...);

/*
 * Fields of the following structures are in the correlation with Yandex.Server configuration file directives.
 */

typedef
struct DOCFORMAT {        /* see the description of Yandex.Server config section 'DocFormat' */
    const char *MimeType; /* document format identifier */
    const char *Config;   /* the string passed to parser */
} DOCFORMAT;

typedef
struct DATASRC {        /* see the description of Yandex.Server config section 'DataSrc' */
    const char *DsId;   /* identifier for data source object */
    const char *Name;   /* name of data source object */
    const char *Module; /* path to dynamic library with DATASRC_LIB, may be zero for using preloaded symbols */
    const char *Symbol; /* DATASRC_LIB's symbol to load */
    const char *Config; /* the string passed to DATASRC_LIB::Open*Session in the second argument */
} DATASRC;

typedef
enum INDEXING_FLAG {               /* see the description of Yandex.Server config directive 'GlobalOptions' */
    INDFL_DEFAULT             = 0, /* StoreArchive DiscardWordForms IgnoreWordFreqs DiscardStopWords StoreIndexingDate Update */
    INDFL_DISCARDARCHIVE      = 1, /* DiscardArchive */
    INDFL_NO_MORPHOLOGY       = 2,
    INDFL_STORESTOPWORDS      = 8, /* StoreStopWords */
    INDFL_DISCARDINDEXINGDATE = 16,/* DiscardIndexingDate */
    INDFL_REINDEX             = 32,/* Reindex */
    INDFL_ONCEONLYINTID       = 256,
    INDFL_STOREFULLARCHIVE    = 512,
    //                        = 2048,
    INDFL_RESULTTOCOUT        = 4096,
    INDFL_DISCARD_LEMMATIZE_URLS = 8192,
    INDFL_DISCARD_SEGMENTATOR_DATA = 16384,
    INDFL_USE_EXTENDED_PROCESSORS = 32768,
    INDFL_COMPRESS_EXT_INFO = 65536,
} INDEXING_FLAG;

typedef
struct INDEX_CONFIG {
    ui32  PortionDocCount;     /* max count of documents in temporary indexer portion */
    ui32  PortionMaxMemory;    /* max memory used by temporary indexer portion, about 30 kb per doc by default */
    ui32       IndexingFlag;   /* can be any combination of the INDEXING_FLAG members */
    const char *StopWordFile;  /* see the description of Yandex.Server config directive 'StopWordFile' */
    const char *DocProperties; /* see the description of Yandex.Server config directive 'DocProperty' */
    const char *Groups;        /* see the description of Yandex.Server config directive 'Groups' */
    const char *PassageProperties; /* see the description of Yandex.Server config directive 'PassageProperties' */
    const char *newindexdir;   /* часть после последнего слеша является префиксом файла, а не директорией */
    const char *oldindexdir;   /* если присутствует, дескрипторы новых документов берутся из свободных */
    const char *tempdir;       /* по-умолчанию, директория из newindexdir */
    const char *workdir;       /* WorkDir из секции Collection */
    YX_LOGNOTIFY YxLogNotify;
    YX_LOGOBJ LogObj;
    DOCFORMAT *formats;
    size_t formatSize;
    const char *NavigationSource; /* see the description of Yandex.Server config directive 'NavigationSource' */
    const char *PureTrieFile;
    const char *RecognizeLibraryFile;
    const char *NamedGroups;   /* see the description of Yandex.Server config directive 'NamedGroups' */
    TLangMask LangMask; /* Does not include LANG_UNK. If not 0, this lang mask which will be set for all documents */
    i32 GrAttrVersion; /* Create the grouping attribute file (indexaa) with this version. Use -1 to select the latest version. */
    bool NoGrAttrsFinalization; /* If true, use the temporary format (NGrouping::TConfig::Index) for the grouping attribute file (this comes in handy when you want to merge other indexes with the current one later on). */
    bool NoAuxiliaryFiles; /* Don't create auxiliary index files (indexerf, indextf, indexfrq, etc.). */
    bool UseDocId; /* Using DOCINPUT::DocId instead of generating in dsindexer. */
} INDEX_CONFIG;

/* Off-page information that is inaccessible from document content */

typedef
enum GRATTR_TYPE {
    GRATTR_CATEG_NAME,
    GRATTR_CATEG_INT,
    GRATTR_CATEG_INTBIN,
} GRATTR_TYPE;

typedef
enum AUXATTR_TYPE {
    AUXATTR_PARSER_PROPERTY, /* indexator will set some parser properties before reading the document */
} AUXATTR_TYPE;

typedef
enum EXTATTR_TYPE {
    EXTATTR_SEARCH   = 1, /* search attributes for key/inv and query language  */
    EXTATTR_GROUP    = 2, /* grouping attributes for aof/atr and form fields   */
    EXTATTR_PROPERTY = 4, /* document properties for dir/arc and search report */
    EXTATTR_AUX      = 8, /* auxiliary document properties for the tuning of indexator */
} EXTATTR_TYPE;

typedef
enum EXTATTR_TYPE_BITS {
    bSearchAttr = 3,
    bGroupAttr = 2,
    bType = 4,
} EXTATTR_TYPE_BITS;

typedef
enum EXTATTR_TYPE_MASK {
    mSearchAttr = ((1UL<<bSearchAttr) - 1),
    mGroupAttr = ((1UL<<bGroupAttr) - 1),
    mType = ((1UL<<bType) - 1),
} EXTATTR_TYPE_MASK;

typedef
struct EXTATTR {
    const char *Name;
    const char *Value;
    ui32        Type;
} EXTATTR;

inline constexpr ui32 YxEaPack(ui32 at, GRATTR_TYPE gt = GRATTR_CATEG_NAME, ATTR_TYPE st = ATTR_LITERAL) {
    return ui32((at << (bSearchAttr + bGroupAttr)) | (ui32(gt) << bSearchAttr) | ui32(st));
}

inline constexpr ATTR_TYPE YxEaAttrType(ui32 type) {
    return (ATTR_TYPE)(type & mSearchAttr);
}

inline constexpr GRATTR_TYPE YxEaGrattrType(ui32 type) {
    return (GRATTR_TYPE)((type >> bSearchAttr) & mGroupAttr);
}

inline constexpr ui32 YxEaExtType(ui32 type) {
    return ui32((type >> (bSearchAttr + bGroupAttr)) & mType);
}

inline void FakeLogNotify(YX_LOGOBJ, LOG_LEVEL, const char*, ...) {
}

#define YX_NEWDOCID 0xFFFFFFFF
typedef ui32  YX_DOCID;
typedef void *YX_DOCOBJ;

typedef
struct DOCINPUT {
    /* obligatory data */
    YX_DOCOBJ ReaderObj;  /* object with document content  */
    const char *DsDocId;  /* уникальный внешний идентификатор документа, */
                          /* источник данных однозначно определяет документ по этому идентификатору. */
                          /* одновременно этот идентификатор является поисковым атрибутом "##_DOC_ID" */
                          /* длина идентификатора не должна превышать 249 символов */
    const char *MimeType; /* document format identifier */
                          /* должен быть одним из идентификаторов, указанных в INDEX_CONFIG::formats */
    /* optional data */
    const char *Charset;  /* encoding, will be recognized if not defined; */
    const char *UserUrl;  /* свойство "адрес документа" (DP_URL), видимое в результатах поиска */
                          /* одновременно этот идентификатор является поисковым атрибутом "#url" */
                          /* бывший UrlProp */
    time_t ModTime;
    ui64 FeedId;
    ui32 DocId;
    /* off-page attributes */
    EXTATTR *ExtAttrs;
    size_t ExtAttrSize;
} DOCINPUT;

#ifdef __cplusplus
}
#endif
