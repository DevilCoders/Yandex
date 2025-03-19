#include <kernel/search_types/search_types.h>
#include "invcreator.h"
#include "posindex.h"
#include "docdata.h"

#include <kernel/indexer/disamber/disamber.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <library/cpp/charset/wide.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <library/cpp/wordpos/wordpos.h>
#include <ysite/yandex/common/prepattr.h>

#include <util/charset/utf8.h>
#include <util/datetime/base.h>
#include <util/generic/strfcpy.h>
#include <library/cpp/containers/str_hash/str_hash.h>
#include <util/stream/file.h>
#include <util/system/unaligned_mem.h>

namespace NIndexerCore {

namespace NIndexerCorePrivate {

inline bool IsStopWord(const wchar16* token, const HashSet* stopWords) {
    if (stopWords) {
        size_t len = std::char_traits<wchar16>::length(token);
        TTempBuf buf(len + 1);
        char* const data = buf.Data();
        WideToChar(token, len, data, CODES_YANDEX);
        data[len] = 0;
        return stopWords->Has(data);
    }
    return false;
}

inline bool IsGoodAttrName(const char* aname) {
    if ('a' <= aname[0] && aname[0] <= 'z') {
        int i = 1;
        while (i < MAXATTRNAME_BUF && aname[i] != '\x00') {
            if (aname[i] == '_' || 'a' <= aname[i] && aname[i] <= 'z' || '0' <= aname[i] && aname[i] <= '9') {
                ++i;
            } else {
                return false;
            }
        }
        if (i < MAXATTRNAME_BUF && aname[i] == '\x00')
            return true;
    }
    return false;
}

// TODO it should be a global function
inline bool IsValidName(const char* s) {
    for (; *s; ++s) {
        if (!(*s >= 'a' && *s <= 'z') && *s != '_' && !(*s >= '0' && *s <= '9'))
           return false;
    }
    return true;
}

//! @return values are based on measurements in prewalrus/dsindexer for docCount 10,100,1000,10000
//! @see __y_prime_list in util/generic/hash.cpp for valid numbers of buckes (initial hash sizes)
inline size_t GetHashSizeByDocCount(size_t docCount) {
    if (docCount <= 1)
        return 6000;
    else if (docCount <= 10)
        return 49000;
    else if (docCount <= 100)
        return 98000;
    else if (docCount <= 1000)
        return 190000;
    else if (docCount <= 10000)
        return 390000;
    else if (docCount <= 100000)
        return 780000;
    else // > 100000
        return 1570000;
}

class TAttrVal {
private:
    const char* const Val;
public:
    explicit TAttrVal(const char* val)
        : Val(val)
    {
        Y_ASSERT(Val && *Val);
    }
    void CopyTo(char* buffer, size_t bufSize) const {
        Y_ASSERT(buffer && bufSize);
        strfcpy(buffer, Val, bufSize);
    }
};

class TWAttrVal {
private:
    const wchar16* const Val;
    const size_t Len;
public:
    TWAttrVal(const wchar16* val, size_t len)
        : Val(val)
        , Len(len)
    {
        Y_ASSERT(Val && *Val && Len);
        Y_ASSERT(!TWtringBuf(Val, Len).Contains(0x7F) );
    }
    void CopyTo(char* buffer, size_t bufSize) const {
        Y_ASSERT(buffer && bufSize);
        TFormToKeyConvertor c(buffer, bufSize);
        c.ConvertAttrValue(Val, Len);
    }
};

template <typename TValue>
inline void StoreAttribute(TPostingIndex& posIndex, size_t docNumber, const char* name, const TValue& value, bool isLiteral, TPosting pos) {
    Y_ASSERT(name && *name);
    const size_t bufSize = MAXKEY_BUF;
    char buffer[bufSize];
    char* p = buffer;
    *p++ = '#';
    p += strlcpy(p, name, bufSize - (p - buffer));
    Y_ASSERT(p - buffer <= MAXATTRNAME_BUF); // 'name' is checked by IsGoodAttrName() so it can't run out of the buffer
    *p++ = '=';
    if (isLiteral)
        *p++ = '\"';

    const size_t n = p - buffer;
    value.CopyTo(buffer + n, bufSize - n);

    SetPostingRelevLevel(pos, LOW_RELEV);
    pos = pos & INT_N_MAX(DOC_LEVEL_Shift);
    posIndex.StoreAttribute(docNumber, buffer, pos);
}

inline void StoreDateTime(TPostingIndex& posIndex, size_t docNumber, const char *attrname, time_t datetime) {
    char datebuf[MAXKEY_LEN];
    char buffer[MAXKEY_LEN];
    long sec;
    sprint_gm_date(datebuf, datetime, &sec);
    snprintf(buffer, MAXKEY_LEN, "#%s=\"%s", attrname, datebuf);
    sec = sec & INT_N_MAX(DOC_LEVEL_Shift);
    posIndex.StoreAttribute(docNumber, buffer, sec);
}

inline void CopyASCII(const wchar16* s, char* p, size_t n) {
    char* e = p + n - 1;
    while (*s && p != e) {
        Y_ASSERT(*s > 0x20 && *s < 0x7F);
        *p++ = *s++;
    }
    *p = 0;
}

}

using namespace NIndexerCorePrivate;

TInvCreatorConfig::TInvCreatorConfig(size_t docCount, size_t maxMemory /*= 0*/, bool stripKeysOnIndex /*= false*/)
    : DocCount(docCount)
    , MaxMemory(maxMemory)
    , GroupForms(true)
    , StripKeysOnIndex(stripKeysOnIndex)
{
    InternalHashSize = GetHashSizeByDocCount(docCount);
    const size_t adjustedDocCount = docCount < 100 ? 100 : docCount;
    KeyBlockSize = 1024 * adjustedDocCount;
    PosBlockSize = 864 * adjustedDocCount;
    DocBlockSize = 256 * adjustedDocCount;
}

TInvCreator::TInvCreator(const TInvCreatorConfig& cfg)
    : MaxMemory(cfg.MaxMemory)
    , PostingIndex(new TPostingIndex(cfg.KeyBlockSize, cfg.PosBlockSize, cfg.DocBlockSize))
    , Documents(new TDocuments(cfg.DocCount))
    , FeedIds(false)
    , CurDocOffset(YX_NEWDOCID)
    , StripKeysOnIndex(cfg.StripKeysOnIndex)
    , AttrNamesToIgnore(cfg.AttrNamesToIgnore)
{
}

TInvCreator::~TInvCreator() {
}

TInvCreatorDTCallback::TInvCreatorDTCallback(IYndexStorageFactory* sf, const TInvCreatorConfig& cfg)
    : Creator(cfg)
    , StorageFactory(sf)
    , GroupForms(cfg.GroupForms)
{
    if (!!cfg.StopWordFile) {
        try {
            THolder<HashSet> temp(new HashSet);
            TFileInput ifs(cfg.StopWordFile);
            temp->Read(&ifs);
            StopWords.Reset(temp.Get());
            Y_UNUSED(temp.Release());
        } catch (...) {
            Cerr << "ignored exception? WTF? " << CurrentExceptionMessage() << Endl;
        }
    }
}

TInvCreatorDTCallback::~TInvCreatorDTCallback() {
}

void TInvCreator::AddDoc() {
    Documents->AddDoc();
    CurDocOffset = Documents->GetLastOffset();
}

void TInvCreator::CommitDoc(ui64 feedId, ui32 docId) {
    Y_ASSERT(CurDocOffset != YX_NEWDOCID);
    if (feedId)
        FeedIds = true;
    Documents->CommitDoc(feedId, docId);
    CurDocOffset = YX_NEWDOCID;
}

bool TInvCreatorDTCallback::ProcessDirectText(const TDirectTextData2& directText, const TDocInfoEx* docInfo,
    const TFullDocAttrs* docAttrs, ui32 docId, const TDisambMask* masks)
{
    if (docId == YX_NEWDOCID)
        return false;

    Creator.AddDoc();
    Creator.StoreDirectText(directText, masks, StopWords.Get());
    StoreExtSearchData(docAttrs);
    Creator.CommitDoc(docInfo ? docInfo->FeedId : 0, docId);

    if (Creator.IsFull()) {
        MakePortion();
        return true;
    }
    return false;
}

void TInvCreator::StoreDirectText(const TDirectTextData2& directText, const TDisambMask* masks, const HashSet* stopWords) {
    for (size_t i = 0; i < directText.EntryCount; ++i) {
        const TDirectTextEntry2& entry = directText.Entries[i];
        if (!entry.Token)
            continue;
        if (IsStopWord(entry.Token.data(), stopWords))
            continue;
        for (size_t j = 0; j < entry.LemmatizedTokenCount; ++j) {
            if (!masks || !masks[i].Mask.Get(j)) {
                const TLemmatizedToken& lemTok = entry.LemmatizedToken[j];
                StoreCachedLemma(lemTok, entry.Posting);
            }
        }
    }

    for (size_t i = 0; i < directText.ZoneCount; ++i) {
        const TDirectTextZone& dtZone = directText.Zones[i];
        if (!IsGoodAttrName(dtZone.Zone.data())) //TODO: move to dtcreator
            continue;
        if (!(dtZone.ZoneType & DTZoneSearch))
            continue;
        for (size_t j = 0; j < dtZone.SpanCount; ++j) {
            const TZoneSpan& span = dtZone.Spans[j];
            TPosting begin;
            SetPosting(begin, span.SentBeg, span.WordBeg);
            TPosting end;
            SetPosting(end, span.SentEnd, span.WordEnd);
            StoreZone(dtZone.Zone.data(), begin, end);
        }
    }

    for (size_t i = 0; i < directText.ZoneAttrCount; ++i) {
        const TDirectTextZoneAttr& attr = directText.ZoneAttrs[i];
        if (!IsGoodAttrName(attr.AttrName.data()))
            continue;
        if (!(attr.AttrType & DTAttrSearchMask))
            continue;
        for (size_t j = 0; j < attr.EntryCount; ++j) {
            const TDirectAttrEntry& entry = attr.Entries[j];
            TPosting pos;
            SetPosting(pos, entry.Sent, entry.Word);
            StoreAttr(attr.AttrType, attr.AttrName.data(), entry.AttrValue.data(), pos);
        }
    }
}

void TInvCreator::StoreCachedLemma(const TLemmatizedToken& ke, TPosting pos) {
    PostingIndex->StoreWord(CurDocOffset, ke, pos, StripKeysOnIndex);
}

void TInvCreator::StoreLiteralAttr(const char* name, const char* value, TPosting pos) {
    char preparedValue[MAXKEY_BUF];
    size_t valueLen = PrepareLiteral(value, preparedValue);
    if (valueLen)
        StoreAttribute(*PostingIndex, CurDocOffset, name, TAttrVal(preparedValue), true, pos);
}

void TInvCreator::StoreLiteralAttr(const char* name, const wchar16* value, TPosting pos) {
    StoreAttr(DTAttrSearchLiteral, name, value, pos);
}

void TInvCreator::StoreIntegerAttr(const char* name, const char* value, TPosting pos) {
    char preparedValue[MAXKEY_BUF];
    size_t valueLen = PrepareInteger(value, preparedValue);
    if (valueLen)
        StoreAttribute(*PostingIndex, CurDocOffset, name, TAttrVal(preparedValue), false, pos);
}

void TInvCreator::StoreDateTimeAttr(const char* name, time_t datetime) {
    StoreDateTime(*PostingIndex, CurDocOffset, name, datetime);
}

void TInvCreator::StoreUrlAttr(const char* name, const wchar16* value, TPosting pos) {
    StoreAttr(DTAttrSearchUrl, name, value, pos);
}

void TInvCreator::StoreKey(const char* key, TPosting pos) {
    PostingIndex->StoreAttribute(CurDocOffset, key, pos);
}

void TInvCreator::StoreZone(const char* name, TPosting beg, TPosting end) {
    Y_ASSERT(CurDocOffset != YX_NEWDOCID);
    Y_ASSERT(IsValidName(name));
    SetPostingRelevLevel(beg, LOW_RELEV);
    SetPostingRelevLevel(end, BEST_RELEV);
    char buffer[MAXKEY_BUF];
    buffer[0] = '(';
    int prefixSize = 1;
    strfcpy(&buffer[prefixSize], name, MAXKEY_BUF - prefixSize);
    PostingIndex->StoreAttribute(CurDocOffset, buffer, beg);
    buffer[0] = ')';
    PostingIndex->StoreAttribute(CurDocOffset, buffer, end);
}

void TInvCreator::StoreExternalLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, ui8 lang, TPosting pos) {
    PostingIndex->StoreExternalLemma(CurDocOffset, lemma, lemmaLen, form, formLen, flags, lang, pos, StripKeysOnIndex);
}

void TInvCreator::StoreZones(const char* name, const TPosting* postings, const TPosting* endOfPostings) {
    Y_ASSERT(CurDocOffset != YX_NEWDOCID);
    Y_ASSERT(IsValidName(name) && ((endOfPostings - postings) & 1) == 0); // postings must contain beginning and end
    char buffer[MAXKEY_BUF];
    const size_t prefixSize = 1;
    strfcpy(&buffer[prefixSize], name, MAXKEY_BUF - prefixSize);
    while (postings < endOfPostings) {
        buffer[0] = '(';
        TPosting beg = *postings++;
        SetPostingRelevLevel(beg, LOW_RELEV);
        PostingIndex->StoreAttribute(CurDocOffset, buffer, beg);
        buffer[0] = ')';
        TPosting end = *postings++;
        SetPostingRelevLevel(end, BEST_RELEV);
        PostingIndex->StoreAttribute(CurDocOffset, buffer, end);
    }
}

void TInvCreator::StoreAttr(ui8 attrType, const char* attrName, const wchar16* attrText, TPosting pos) {
    Y_ASSERT(CurDocOffset != YX_NEWDOCID);
    EDTAttrType atype = (EDTAttrType)(attrType & DTAttrSearchMask);

    if (AttrNamesToIgnore.contains(attrName)) {
        return;
    }

    switch (atype) {
        case DTAttrSearchLiteral:
        {
            wchar16 preparedValue[MAXKEY_BUF];
            size_t valueLen = PrepareLiteral(attrText, preparedValue);
            if (valueLen)
                StoreAttribute(*PostingIndex, CurDocOffset, attrName, TWAttrVal(preparedValue, valueLen), true, pos);
        }
        break;
        case DTAttrSearchDate:
        {
            TString s = WideToChar(TWtringBuf(attrText), CODES_YANDEX);
            char *ptr;
            ui32 tm = strtoul(s.data(), &ptr, 10);
            if (*ptr == 0)
                StoreDateTime(*PostingIndex, CurDocOffset, attrName, tm);
        }
        break;
        case DTAttrSearchInteger:
        {
            wchar16 preparedValue[MAXKEY_BUF];
            size_t valueLen = PrepareInteger(attrText, preparedValue);
            if (valueLen)
                StoreAttribute(*PostingIndex, CurDocOffset, attrName, TWAttrVal(preparedValue, valueLen), false, pos);
        }
        break;
        case DTAttrSearchUrl:
        {
            if (strnicmp(attrName, "img", 3) == 0) {
                wchar16 preparedValue[MAXKEY_BUF];
                size_t valueLen = PrepareURL(attrText, preparedValue);
                if (valueLen)
                    StoreAttribute(*PostingIndex, CurDocOffset, attrName, TWAttrVal(preparedValue, valueLen), true, pos);
                //PPB hack: img attribute is indexed twice.
            }
            {
                TTempBuf buf;
                CopyASCII(attrText, buf.Data(), buf.Size()); // TODO remove this copying, pass const char* attrText
                const char* p = buf.Data();
                if (strnicmp(attrName, "link", 4) != 0 && strnicmp(attrName, "url", 3) != 0) {
                    //TODO: this 'if' should depend on attribute _type_ instead of attribute _name_!
                    if (strchr(p, '?'))
                        return; //drop a cgi url
                    p = strrchr(p, '/'); //skip server and path
                    if (!p)
                        return; //drop an url with no '/'
                    ++p; //skip '/' symbol
                    if (*p == 0)
                        return; //drop an empty filename
                }
                char preparedValue[MAXKEY_BUF];
                if (PrepareURL(p, preparedValue))
                    StoreAttribute(*PostingIndex, CurDocOffset, attrName, TAttrVal(preparedValue), true, pos);
            }
        }
        break;
        default:
        break;
    }
}

bool TInvCreator::IsFull() const {
    if (Documents->IsFull())
        return true;
    if (MaxMemory) {
        // we do not take into account the tails of segments
        size_t usage = PostingIndex->MemUsage()
                     + Documents->MemUsage();
        return usage >= MaxMemory;
    }
    return false;
}

void TInvCreatorDTCallback::StoreExtSearchData(const TFullDocAttrs* docAttrs) {
    if (!docAttrs)
        return;
    for (TFullDocAttrs::TConstIterator i = docAttrs->Begin(); i != docAttrs->End(); ++i) {
        if (i->Type & TFullDocAttrs::AttrSearchZones) {
            Y_ASSERT(i->Value.size() % (sizeof(TPosting) * 2) == 0);
            const TPosting* postings = reinterpret_cast<const TPosting*>(i->Value.data());
            const TPosting* endOfPostings = reinterpret_cast<const TPosting*>(i->Value.data() + i->Value.size());
            Creator.StoreZones(i->Name.data(), postings, endOfPostings);
            continue;
        }
        if (i->Type & TFullDocAttrs::AttrSearchLitPos || i->Type & TFullDocAttrs::AttrSearchIntPos) {
            ui8* beg = (ui8*)i->Value.data();
            ui8* end = beg + i->Value.size();
            while (beg < end) {
                TPosting pos = ReadUnaligned<TPosting>(beg);
                beg += sizeof(TPosting);
                ui16 len = ReadUnaligned<ui16>(beg);
                beg += sizeof(ui16);
                Y_ASSERT(beg[len] == 0); // it must be null-terminated
                if (i->Type & TFullDocAttrs::AttrSearchIntPos) {
                    TUtf16String value = CharToWide<false>((const char*)beg, len, CODES_YANDEX);
                    Creator.StoreAttr(DTAttrSearchInteger, i->Name.data(), value.data(), pos);
                    beg += len + 1;
                } else if (i->Type & TFullDocAttrs::AttrSearchLitPos) {
                    if (len == 0) {
                        pos = pos & INT_N_MAX(DOC_LEVEL_Shift);
                        Creator.StoreKey(i->Name.data(), pos);
                        beg += 1;
                    } else {
                        TUtf16String value = (*beg == UTF8_FIRST_CHAR ? UTF8ToWide((const char*)beg + 1, len - 1) : CharToWide<false>((const char*)beg, len, CODES_YANDEX));
                        Creator.StoreAttr(DTAttrSearchLiteral, i->Name.data(), value.data(), pos);
                        beg += len + 1;
                    }
                }
            }
            Y_ASSERT(beg == end);
            continue;
        }

        if (i->Type & TFullDocAttrs::AttrSearchLemma) {
            ui8* beg = (ui8*)i->Value.data();
            ui8* end = beg + i->Value.size();
            while (beg < end) {
                TPosting pos = *((TPosting*)beg);
                beg += sizeof(TPosting);
                ui8 flag = *((ui8*)beg);
                beg += sizeof(ui8);
                ui8 lang = *((ui8*)beg);
                beg += sizeof(ui8);
                ui16 len = *((ui16*)beg);
                beg += sizeof(ui16);
                TString forma((const char*)beg, len);
                TString lemma = i->Name;
                TUtf16String l, f;
                if (*lemma.data() == UTF8_FIRST_CHAR) {
                    l = UTF8ToWide(lemma.data() + 1, lemma.size() - 1);
                    f = UTF8ToWide(forma);
                } else {
                    l = CharToWide(lemma, csYandex);
                    f = CharToWide(forma, csYandex);
                }
                Creator.StoreExternalLemma(l.data(), l.size(), f.data(), f.size(), flag, lang, pos);
                beg += len;
            }
            Y_ASSERT(beg == end);
            continue;
        }

        TUtf16String wvalue;
        const size_t utf8Position = i->Value.find(UTF8_FIRST_CHAR);
        if (utf8Position == TString::npos) {
            wvalue = CharToWide(i->Value, csYandex);
        } else {
            const TStringBuf charPart(i->Value.data(), utf8Position);
            const TStringBuf utf8Part(i->Value.data() + utf8Position + 1);
            wvalue = (UTF8Detect(utf8Part) == UTF8) ? CharToWide(charPart, csYandex) + UTF8ToWide(utf8Part) : CharToWide(i->Value, csYandex);
        }

        if (i->Type & TFullDocAttrs::AttrSearchDate) {
            Creator.StoreAttr(DTAttrSearchDate, i->Name.data(), wvalue.data(), 0); // the last argument is unused here
        }
        if (i->Type & TFullDocAttrs::AttrSearchLiteral) {
            Creator.StoreAttr(DTAttrSearchLiteral, i->Name.data(), wvalue.data(), 0);
        }
        if (i->Type & TFullDocAttrs::AttrSearchUrl) {
            Creator.StoreAttr(DTAttrSearchUrl, i->Name.data(), wvalue.data(), 0);
        }
        if (i->Type & TFullDocAttrs::AttrSearchInteger) {
            Creator.StoreAttr(DTAttrSearchInteger, i->Name.data(), wvalue.data(), 0);
        }
    }
}

}
