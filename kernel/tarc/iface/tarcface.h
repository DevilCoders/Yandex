#pragma once

#include "arcface.h"
#include "comp_tables.h"

#include <util/system/defaults.h>
#include <util/stream/output.h>
#include <util/stream/buffer.h>
#include <util/stream/mem.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>
#include <util/memory/blob.h>
#include <util/generic/buffer.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>

/*
doctext:
||header: sizeof(TArchiveTextHeader)|markupinfo: InfoLen|blocksdir: BlockCount*sizeof(TArchiveTextBlockInfo)|blocks: GetTextBlocksLen()||
*/

struct Y_PACKED TArchiveTextHeader {
    ui32 InfoLen;    // длина в байтах блока 'markupinfo' с информацией о зонах разметки
    ui16 BlockCount; // количество блоков
    ui16 Reserved;
    TArchiveTextHeader()
        : InfoLen(0)
        , BlockCount(0)
        , Reserved(0)
    {}
};

Y_DECLARE_PODTYPE(TArchiveTextHeader);

// Блоки с нулевым TArchiveTextBlockInfo::BlockFlag всегда должны быть первыми,
// чтобы не нарушить нумерацию предложений

static constexpr ui16 BLOCK_IS_MAIN = 0;     // содержит основной текст документа
static constexpr ui16 BLOCK_IS_EXTENDED = 1; // содержит дополнительные блоки текста (видео-сниппеты и т.п.)

struct Y_PACKED TArchiveTextBlockInfo {
    ui32 EndOffset; // смещение в байтах конца блока относительно начала массива блоков
    ui16 SentCount; // количество предложений в блоке
    ui16 BlockFlag; // комбинация BLOCK_*
    TArchiveTextBlockInfo()
        : EndOffset(0)
        , SentCount(0)
        , BlockFlag(BLOCK_IS_MAIN)
    {}
};

Y_DECLARE_PODTYPE(TArchiveTextBlockInfo);

inline ui32 GetTextBlocksLen(ui32 textLen, const TArchiveTextHeader& th) {
    return textLen - sizeof(TArchiveTextHeader) - th.InfoLen - th.BlockCount * sizeof(TArchiveTextBlockInfo);
}

void AddExtendedBlock(const TBlob& origText, const char* block, size_t blSize, TBuffer* toWrite, bool replace = false);

// граница блока всегда совпадает с границей предложения,
// но может попасть в середину абзаца

static constexpr ui16 SENT_IS_PARABEG = 1;      // является началом абзаца
static constexpr ui16 SENT_IS_PARAEND = 2;      // является концом абзаца
static constexpr ui16 SENT_HAS_MARKUP_ZONE = 4; // имеются слова из парсерных зон
static constexpr ui16 SENT_HAS_WEIGHT_ZONE = 8; // имеются слова с RelevLevel == LOW_RELEV, HIGH_RELEV или BEST_RELEV
static constexpr ui16 SENT_HAS_ATTRS = 16;      // имеются атрибуты предложения
static constexpr ui16 SENT_HAS_EXTSYMBOLS = 32; // имеются символы не из csYandex, сохранено как wchar16

struct TArchiveTextBlockSentInfo {
    ui32 OrigOffset; // смещение в байтах исходного предложения от начала исходного документа
    ui16 EndOffset;  // смещение в байтах конца предложения относительно начала массива предложений
    ui16 Flag;       // комбинация SENT_*
    TArchiveTextBlockSentInfo()
        : OrigOffset(0)
        , EndOffset(0)
        , Flag(0)
    {}
};

Y_DECLARE_PODTYPE(TArchiveTextBlockSentInfo);

void SaveArchiveTextBlockSentInfos(
        TConstArrayRef<TArchiveTextBlockSentInfo> infos,
        IOutputStream* out);

inline void SaveArchiveTextBlockSentInfos(
        const TVector<TArchiveTextBlockSentInfo>& infos,
        IOutputStream* out) {
    return SaveArchiveTextBlockSentInfos(TConstArrayRef<TArchiveTextBlockSentInfo>{infos}, out);
}

void LoadArchiveTextBlockSentInfos(
        TMemoryInput* in,
        TStringBuf* infos,
        TVector<char>* tempBuffer);

// в даной версии поддерживаем только статические имена зон, чтобы упростить код мержевания
enum EArchiveZone {
    AZ_TITLE =        0,
    AZ_ANCHOR =       1,
    AZ_ANCHORINT =    2,
    AZ_ABSTRACT =     3,
    AZ_KEYWORDS =     4,
    AZ_HINT =         5,
    AZ_ADDRESS =      6,
    AZ_SEGAUX =       7,
    AZ_SEGCONTENT =   8,
    AZ_SEGCOPYRIGHT = 9,  // SEGFOOTER
    AZ_SEGHEAD =      10,
    AZ_SEGLINKS =     11,
    AZ_SEGMENU =      12,
    AZ_SEGNEWS =      13, // deprecated, delete
    AZ_SEGREFERAT =   14,
    AZ_SEGTITLE =     15, // deprecated, delete
    AZ_TEXTAREA = 16,
    AZ_TELEPHONE =    17,
    AZ_ARTICLE = 18,
    AZ_HEADER = 19,
    AZ_MAIN_CONTENT = 20,
    AZ_MAIN_HEADER = 21,
    AZ_DATER_DATE = 22,
    AZ_FAQ_Q =        23,
    AZ_FAQ_A =        24,
    AZ_HANSWER_Q = 25, // hfeed-faq microformat
    AZ_HANSWER_A = 26,
    AZ_HANSWER_TITLE = 27,
    AZ_HANSWER_CONTENT = 28,
    AZ_FIO = 29,
    AZ_FORUM_MESSAGE = 30,
    AZ_FORUM_QUOTE = 31,
    AZ_FORUM_SIGNATURE = 32,
    AZ_OPINION = 33,
    AZ_XPATH = 34,
    AZ_USER_REVIEW = 35,
    AZ_FORUM_QBODY = 36,
    AZ_STRICT_HEADER = 37,
    AZ_TABLE = 38,
    AZ_TABLE_ROW = 39,
    AZ_TABLE_CELL = 40,
    AZ_LIST0 = 41,
    AZ_LIST1 = 42,
    AZ_LIST2 = 43,
    AZ_LIST3 = 44,
    AZ_LIST4 = 45,
    AZ_LIST5 = 46,
    AZ_LIST_ITEM0 = 47,
    AZ_LIST_ITEM1 = 48,
    AZ_LIST_ITEM2 = 49,
    AZ_LIST_ITEM3 = 50,
    AZ_LIST_ITEM4 = 51,
    AZ_LIST_ITEM5 = 52,
    AZ_FORUM_INFO = 53,
    AZ_FORUM_TOPIC_INFO = 54,
    AZ_MEDIAWIKI_TITLE = 55,
    AZ_MEDIAWIKI_HEADLINE = 56,
    AZ_MEDIAWIKI_AUX = 57,
    AZ_MEDIAWIKI_TOC = 58,
    AZ_MEDIAWIKI_INFOBOX = 59,
    AZ_MEDIAWIKI_REFS = 60,
    AZ_MEDIAWIKI_CONTENT = 61,
    AZ_MEDIAWIKI_CATS = 62,
    AZ_MEDIAWIKI_QUOTE = 63,
    AZ_MEDIAWIKI_THUMB = 64,
    AZ_MEDIAWIKI_GALLERY = 65,
    AZ_MEDIAWIKI_NAVBOX = 66,
    AZ_MEASURE = 67,
    AZ_ALTERNATE = 68,
    AZ_MEDIAWIKI_TEXT = 69,
    AZ_NOINDEX = 70,
    AZ_COUNT
};

const char* ToString(EArchiveZone id);
bool IsAttrAllowed(EArchiveZone zone, const char* attrName);

namespace NArchiveZoneAttr {
    namespace NAnchor {
        extern const char* LINK;
    }
    namespace NAnchorInt {
        extern const char* LINKINT;
        extern const char* SEGMLINKINT; // This attribute is using for "links in snippets" project SNIPPETS-1431
        // If you add something here, you should change the way of storing urls in the archive (TUrlData representation).
    }
    namespace NTelephone {
        extern const char* COUNTRY_CODE;
        extern const char* AREA_CODE;
        extern const char* LOCAL_NUMBER;
    }
    namespace NSegm {
        extern const char* STATISTICS;
    }
    namespace NForum {
        // message attributes
        extern const char* DATE;
        extern const char* AUTHOR;
        extern const char* ANCHOR;
        // quote attributes
        extern const char* QUOTE_DATE;
        extern const char* QUOTE_TIME;
        extern const char* QUOTE_AUTHOR;
        extern const char* QUOTE_URL;
        // subforum/topic info attributes
        // one item cannot be both message and subforum and topic info,
        // so it's ok to share some attributes
        extern const char* NAME;
        extern const char* LINK;
        extern const char* DESCRIPTION;
        extern const char* TOPICS;
        extern const char* MESSAGES;
        extern const char* VIEWS;
        extern const char* LAST_TITLE;
        extern const char* LAST_AUTHOR;
        extern const char *LAST_DATE;
    }
    namespace NXPath {
        extern const char* XPATH_ZONE_TYPE;
    }
    namespace NDater {
        extern const char* DATE;
    }
    namespace NUserReview {
        extern const char* AUTHOR;     // автор отзыва
        extern const char* DATE;       // дата отзыва (как выглядит на странице)
        extern const char* NORM_DATE;  // нормализованная дата (20120101)
        extern const char* RATE;       // рейтинг
        extern const char* TITLE;      // заголовок
        extern const char* USEFULNESS; // полезность
    }
    namespace NFio {
        extern const char* FIO_LEN;
    }
    namespace NNumber {
        extern const char* MEASURENUM;
    }
}

struct TArchiveZoneSpan {
    ui16 SentBeg;   // номер предложения, в котором начинается зона
    ui16 OffsetBeg; // смещение в _символах_ начала зоны от начала предложения
    ui16 SentEnd;   // номер предложения, в котором заканчивается зона
    ui16 OffsetEnd; // смещение в _символах_ конца зоны от начала предложения
    TArchiveZoneSpan()
        : SentBeg(0)
        , OffsetBeg(0)
        , SentEnd(0)
        , OffsetEnd(0)
    {
    }

    bool Empty() const {
        return SentBeg == SentEnd && OffsetBeg == OffsetEnd;
    }
};

Y_DECLARE_PODTYPE(TArchiveZoneSpan);

typedef THashMap<ui32, THashMap<TString, TUtf16String> > TSpan2Attrs;
typedef THashMap<ui32, TString> TSpan2SegmentAttrs;

template <typename TAttrsHashPtr, typename TSegmentAttrsPtr>
struct TSpanAttributesImpl {
    TAttrsHashPtr AttrsHash;
    TSegmentAttrsPtr SegmentAttrs;
    TSpanAttributesImpl()
        : AttrsHash(nullptr)
        , SegmentAttrs(nullptr)
    {}
};

typedef TSpanAttributesImpl<THashMap<TString, TUtf16String>*, TString* > TSpanAttributes;
typedef TSpanAttributesImpl<const THashMap<TString, TUtf16String>*, const TString* > TConstSpanAttributes;

class TArchiveZoneAttrs {
public:
    union TCoord {
        ui32 Coord;
        struct {
            ui16 Sent;
            ui16 Off;
        };
        TCoord(ui16 s, ui16 o) {
            Sent = s;
            Off = o;
        }
        TCoord(ui32 c) {
            Coord = c;
        }
    };
    union TZoneCoord {
        ui64 ZoneCoord;
        struct {
            ui32 ZoneId;
            ui32 Coord;
        };
        TZoneCoord(ui32 z, ui32 c)
        {
            ZoneId = z;
            Coord = c;
        }
        TZoneCoord(ui64 zc) {
            ZoneCoord = zc;
        }
    };
    struct TSpan2AttrsWrapper
    {
        TSpan2Attrs Span2Attrs;
        TSpan2SegmentAttrs Span2SegmentAttrs;

        TConstSpanAttributes GetSpanAttrs(ui16 sent, ui16 off) const
        {
            TConstSpanAttributes res;
            TSpan2Attrs::const_iterator attrIt = Span2Attrs.find(TCoord(sent, off).Coord);
            if (attrIt != Span2Attrs.end()) {
                res.AttrsHash = &attrIt->second;
            }
            TSpan2SegmentAttrs::const_iterator segAttrIt = Span2SegmentAttrs.find(TCoord(sent, off).Coord);
            if (segAttrIt != Span2SegmentAttrs.end()) {
                res.SegmentAttrs = &segAttrIt->second;
            }
            return res;
        }
        TSpanAttributes GetSpanAttrs(ui16 sent, ui16 off)
        {
            TSpanAttributes res;
            TSpan2Attrs::iterator attrIt = Span2Attrs.find(TCoord(sent, off).Coord);
            if (attrIt != Span2Attrs.end()) {
                res.AttrsHash = &attrIt->second;
            }
            TSpan2SegmentAttrs::iterator segAttrIt = Span2SegmentAttrs.find(TCoord(sent, off).Coord);
            if (segAttrIt != Span2SegmentAttrs.end()) {
                res.SegmentAttrs = &segAttrIt->second;
            }
            return res;
        }
        bool Empty() const
        {
            return Span2Attrs.empty() && Span2SegmentAttrs.empty();
        }
    };

private:
    TSpan2AttrsWrapper Span2AttrsWrapper;
public:
    ui32 ZoneId;
    TSpanAttributes GetSpanAttrs(const TArchiveZoneSpan& span);
    TConstSpanAttributes GetSpanAttrs(const TArchiveZoneSpan& span) const;
    TSpanAttributes GetSpanAttrs(ui16 sent, ui16 off);
    TConstSpanAttributes GetSpanAttrs(ui16 sent, ui16 off) const;

    void AddSpanAttr(ui16 sent, ui16 off, const char* name, const wchar16* val);
    void AddSpanAttr(ui32 sentAndOff, const TString& name, const TUtf16String& val);
    void SwapAttrs(ui16 sent, ui16 off, TSpanAttributes& spanAttributes);
    void Swap(TArchiveZoneAttrs& another);
    void Save(IOutputStream* out, TMultiMap<TUtf16String, ui64>& urls, ui8 segVersion) const;
    void Load(IInputStream* in, ui8 arcZoneAttrVersion);
    void LoadProtobuf(IInputStream* in);
    bool Empty() const;
};

struct TArchiveZone {
    ui32 ZoneId;
    TVector<TArchiveZoneSpan> Spans;

    void SaveAsIs(IOutputStream* out, bool saveId = true) const;
    void SaveDelta(IOutputStream* out, bool saveId = true) const;
    void LoadAsIs(IInputStream* in, bool loadId = true);
    void LoadDelta(IInputStream* in, bool loadId = true);
};

class TArchiveMarkupZones {
public:
    TVector<TArchiveZone> Zones;
    TVector<TArchiveZoneAttrs> Attrs;
    ui8 ArcZoneAttrVersion;
    ui8 SegVersion;

private:
    void SaveVersionLabel(IOutputStream* out) const;
    void SaveArchiveZones(IOutputStream* out) const;
    void SaveArchiveZonesAsIs(IOutputStream* out) const;
    void SaveArchiveZonesDelta(TVector<char>* out, ui32 expectedBytes) const;
    void SaveArchiveZoneAttrs(
            IOutputStream* out,
            TMultiMap<TUtf16String, ui64>& urls) const;
    void SaveUrlTrie(
            IOutputStream* out,
            const TMultiMap<TUtf16String, ui64> &urls) const;

    void LoadAttrVersion(TMemoryInput* in);
    void LoadArchiveZonesAsIs(TMemoryInput* in);
    void LoadArchiveZonesDelta(TMemoryInput* in);
    void LoadArchiveZoneAttrs(TMemoryInput* in);

public:
    TArchiveMarkupZones();

    void Save(IOutputStream* out) const;
    void Load(TMemoryInput* in);

    ui8 GetSegVersion() const;
    const TArchiveZone& GetZone(ui32 id) const;
    TArchiveZone& GetZone(ui32 id);
    const TArchiveZoneAttrs& GetZoneAttrs(ui32 id) const;
    TArchiveZoneAttrs& GetZoneAttrs(ui32 id);
};

struct TArchiveWeightZones {
    TArchiveZone LowZone;
    TArchiveZone HighZone;
    TArchiveZone BestZone;

    TArchiveWeightZones() {
        LowZone.ZoneId = 0;
        HighZone.ZoneId = 2;
        BestZone.ZoneId = 3;
    }

    void Clear() {
        LowZone.Spans.clear();
        HighZone.Spans.clear();
        BestZone.Spans.clear();
    }

    void Save(IOutputStream* rh) const;
    void Load(TMemoryInput* in);

    static void Skip(IInputStream* in) {
        ui32 len = 0;
        in->LoadOrFail(&len, sizeof(len));
        ui32 realLen = in->Skip(len);
        Y_ASSERT(realLen == len);
    }

private:
    void SaveAsIs(TBufferOutput* out) const;
    void SaveDelta(TVector<char>* out, ui32 expectedSize) const;
    void LoadAsIs(IInputStream* in);
    void LoadDelta(TMemoryInput* in, ui32 len);
};

class TZoneSpanIterator
{
protected:
    TArchiveZoneSpan* CurSpan;
    TArchiveZoneSpan* EndSpan;
    TArchiveZoneAttrs* Attrs;
    bool BegState;
public:
    TZoneSpanIterator(TArchiveZoneSpan* cur, TArchiveZoneSpan* end, TArchiveZoneAttrs* attrs)
        : CurSpan(cur)
        , EndSpan(end)
        , Attrs(attrs)
        , BegState(true)
    {
    }

    typedef ui32 value_type;

    void Next() {
        if (!Valid())
            return;
        if (BegState) {
            BegState = false;
        } else {
            ++CurSpan;
            BegState = true;
        }
    }

    bool Valid() const {
        return CurSpan != EndSpan;
    }

    ui32 Current() const {
        return BegState ? ((CurSpan->SentBeg << 16) | CurSpan->OffsetBeg)
                        : ((CurSpan->SentEnd << 16) | CurSpan->OffsetEnd);
    }

    TSpanAttributes GetCurrentAttrs() const {
        if (!Attrs)
            return TSpanAttributes();
        return Attrs->GetSpanAttrs(*CurSpan);
    }

    void operator ++() {
        Next();
    }

    void Restart() const {
    }
};


// https://wiki.yandex-team.ru/Robot/AddNewSources#3.4dannyeposnippetam
#define ARCHIVE_FIELD_VALUE_LIST_SEP "\x07;"

// empty string if not found
TString ParamFromAttrValue(const TString& value, const TString& attrName);
TString ParamFromArcHeader(const THashMap<TString, TString>& headers, const TString& ns, const TString& attrName);
