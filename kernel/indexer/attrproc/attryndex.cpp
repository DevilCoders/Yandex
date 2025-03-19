#include <util/folder/dirut.h>
#include <util/generic/hash_set.h>
#include <kernel/search_types/search_types.h>

#include <kernel/keyinv/invkeypos/keynames.h>
#include <ysite/yandex/common/prepattr.h>
#include <kernel/keyinv/invkeypos/keychars.h>

#include "attryndex.h"

static const char* const DomainName = "domain";
static const char* const UpathName = "upath";
static const char* const LupathName = "lupath";
static const char* const RupathName = "rupath";

TAttrStacker::TAttrStacker()
    : Inserter(nullptr)
{
    AttrPosHash.insert(TAttrPosHash::value_type(DomainName, 0));
    AttrPosHash.insert(TAttrPosHash::value_type(UpathName, 0));
    AttrPosHash.insert(TAttrPosHash::value_type(LupathName, 0));
    AttrPosHash.insert(TAttrPosHash::value_type(RupathName, 0));
}

void TAttrStacker::Clear() {
    for (TAttrPosHash::iterator i = AttrPosHash.begin(); i != AttrPosHash.end(); ++i)
        i->second = 0;
}

inline TPosting TAttrStacker::GetNextAttrPos(const char *name) {
    TAttrPosHash::iterator i = AttrPosHash.find(name);
    if (i == AttrPosHash.end())
        return 0;
    if (i->second < WORD_LEVEL_Max)
        i->second++;
    TPosting pos = 0;
    SetPosting(pos, 0, i->second, LOW_RELEV);
    return pos;
}

void TAttrStacker::MoveAttrPos(const char* name) {
    TAttrPosHash::iterator i = AttrPosHash.find(name);
    if (i != AttrPosHash.end() && i->second < WORD_LEVEL_Max)
        i->second++;
}

void TAttrStacker::StoreUrl(const TString& url, ELanguage lang, const TBlob* tokSplitData) {
    Y_ASSERT(Inserter);

    TUrlTransliterator transliterator(TransliteratorCache, url, lang, tokSplitData);
    while (transliterator.Has()) {
        TUrlTransliteratorItem item;
        transliterator.Next(item);
        if (!StoreUrlTransliteratorItem(item)) {
            break;
        }
    }
}

void TAttrStacker::StoreUrl(const TString& url, ELanguage lang, const TVector<TString>& urlTransliterationData, const TBlob* tokSplitData) {
    Y_ASSERT(Inserter);
    if (urlTransliterationData.empty()) {
        StoreUrl(url, lang, tokSplitData);
    } else {
        for (const TString& itemStr : urlTransliterationData) {
            TUrlTransliteratorItem item;
            TStringInput in(itemStr);
            ::Load(&in, item);
            if (!StoreUrlTransliteratorItem(item)) {
                break;
            }
        }
    }
}

bool TAttrStacker::StoreUrlTransliteratorItem(const TUrlTransliteratorItem& item) {
    const ui32 wordIndex = item.GetIndex() + TWordPosition::FIRST_CHILD;
    if (wordIndex >= (1 << WORD_LEVEL_Bits))
        return false;
    TPosting posting;
    SetPosting(posting, 0, wordIndex, MID_RELEV);
    const TUtf16String& form = item.GetForma();
    const TUtf16String& lemma = item.GetLowerCaseLemma();
    ui8 flags = 0;
    if (item.IsPartTitled())
        flags |= FORM_TITLECASE;
    if (item.GetLemmaType() == TUrlTransliteratorItem::LtTranslit)
        flags |= FORM_TRANSLIT;
    Inserter->StoreLemma(lemma.c_str(), lemma.size(), form.c_str(), form.size(), flags, posting, item.GetLanguage());
    return true;
}

void TAttrStacker::StoreDocAttrLit(const char* attrname, const char* attrValue) {
    TUtf16String value(UTF8ToWide(attrValue));
    StoreDocAttrLit(attrname, value.data());
}

void TAttrStacker::StoreDocAttrLit(const char* attrname, const wchar16* attrValue) {
    Y_ASSERT(attrname && *attrname);
    Y_ASSERT(attrValue && *attrValue);
    Y_ASSERT(Inserter);
    TPosting pos = GetNextAttrPos(attrname);
    Inserter->StoreLiteralAttr(attrname, attrValue, std::char_traits<wchar16>::length(attrValue), pos);
}

void TAttrStacker::StoreDocAttrInt(const char *attrname, const char *attrValue) {
    Y_ASSERT(attrname && *attrname);
    Y_ASSERT(attrValue && *attrValue);
    Y_ASSERT(Inserter);

    TPosting pos = GetNextAttrPos(attrname);
    Inserter->StoreIntegerAttr(attrname, attrValue, pos);
}

void TAttrStacker::StoreDocAttrUrl(const char* attrname, const char* attrValue) {
    Y_ASSERT(attrname && *attrname);
    Y_ASSERT(attrValue && *attrValue);
    Y_ASSERT(Inserter);
    wchar16 buf[MAXKEY_BUF];
    size_t len = PrepareURL(attrValue, buf, MAXKEY_BUF - 1);
    buf[len] = 0;
    if (len) {
        TPosting pos = GetNextAttrPos(attrname);
        Inserter->StoreLiteralAttr(attrname, buf, len, pos);
    }
}

void TAttrStacker::StoreDocAttrUrl(const char* attrname, const wchar16* attrValue) {
    Y_ASSERT(attrname && *attrname);
    Y_ASSERT(attrValue && *attrValue);
    Y_ASSERT(Inserter);

    wchar16 buf[MAXKEY_BUF];
    size_t len = PrepareURL(attrValue, buf);
    if (len) {
        TPosting pos = GetNextAttrPos(attrname);
        Inserter->StoreLiteralAttr(attrname, buf, len, pos);
    }
}

void TAttrStacker::StoreDateTime(const char *attrname, time_t datetime) {
    Y_ASSERT(Inserter);
    Inserter->StoreDateTimeAttr(attrname, datetime);
}

void TAttrStacker::StoreZone(const char* name, TPosting begin, TPosting end) {
    Y_ASSERT(Inserter);
    Inserter->StoreZone(name, begin, end);
}
