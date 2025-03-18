#include "attrextractor.h"
#include "ht_conf.h"
#include "findin.h"

#include <cctype>
#include <cstdlib>

#include <algorithm>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>
#include <util/string/util.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/system/compat.h>
#include <util/system/defaults.h>
#include <util/system/maxlen.h>

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/face/event.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/relalternate/relalternate.h>
#include <library/cpp/html/url/url.h>

namespace {
    class TStringType {
        char* /*const*/ Ptr;
        size_t Len;
        TPoolAlloc<char> Alloc;

    public:
        TStringType(const char* s, size_t n, TMemoryPool& buf)
            : Len(n)
            , Alloc(&buf)
        {
            Ptr = Alloc.allocate(Len + 1);
            memcpy(Ptr, s, Len);
            Ptr[Len] = 0;
        }
        TStringType(const char* s1, const char* s2, TMemoryPool& buf)
            : Alloc(&buf)
        {
            size_t n1 = strlen(s1);
            size_t n2 = strlen(s2);
            Len = n1 + n2;
            Ptr = Alloc.allocate(Len + 1);
            memcpy(Ptr, s1, n1);
            memcpy(Ptr + n1, s2, n2);
            Ptr[Len] = 0;
        }
        ~TStringType() {
            Alloc.deallocate(Ptr, 0);
        }
        char* begin() {
            return Ptr;
        }
        const char* c_str() const {
            return Ptr;
        }
        size_t size() const {
            return Len;
        }
        void clear() {
            Len = 0;
        }
        void erase(size_t pos = 0, size_t n = TString::npos) {
            if (pos > size())
                ythrow yexception() << "invalid pos";
            if (n == TString::npos) {
                Len = pos;
            } else {
                size_t xlen = Min(n, Len - pos);
                size_t tailLen = Len - pos - xlen;
                memmove(Ptr + pos, Ptr + pos + xlen, tailLen + 1); // +1 - NTS
                Len = pos + tailLen;
            }
        }
    };
}

static void find_extension(const TString& s, const char*& ext, unsigned& extlen) {
    // ".mp3" suffix ?
    const char* end = strchr(s.data(), '?');
    if (end == nullptr)
        end = s.data() + s.size();
    for (ext = end; ext > s.data() && !strchr(":/\\", ext[-1]); ext--) {
        if (ext[-1] == '.') {
            extlen = unsigned(end - ext);
            break;
        }
    }
}

/// this is the same as strip from util, but a0 is not a space as it may be a part of utf rune
/// actually we should do Strip after correct Decode, but here cp is not set yet
//! @todo it could be used for string buffers and return std::pair<size_t, size_t> - new first position and new length
template <typename T>
static void SoftStrip(T& str) {
    static const bool spaces[33] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
        1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 1};

    const char* p = str.c_str();
    const char* const e = p + str.size();

    while (p != e) {
        const unsigned char c = *p;
        if (c > 32 || !spaces[c])
            break;
        ++p;
    }

    if (p == e) {
        str.clear();
        return;
    }

    const char* rp = e - 1;
    const char* const re = p - 1;

    while (rp != re) {
        const unsigned char c = *rp;
        if (c > 32 || !spaces[c])
            break;
        --rp;
    }

    if (p != str.c_str())
        str.erase(0, p - str.c_str());

    str.erase(rp - re);
}

static inline int FindAtt(const TStringBuf& name, const THtmlChunk& ev) {
    for (unsigned i = 0; i < ev.AttrCount; i++) {
        if (name.size() == ev.Attrs[i].Name.Leng && strnicmp(ev.text + ev.Attrs[i].Name.Start, name.data(), name.size()) == 0) {
            return i;
        }
    }
    return -1;
}

static inline void RenameAttribute(NHtml::TAttribute& name, NHtml::TAttribute& value) {
    name.Name = name.Value;
    name.Value = value.Value;
    value.Name.Leng = 0; // kill
}

// rename some meta attributes in copy of evnt attributes
static inline int CheckMetaRename(const THtmlChunk& ev, NHtml::TAttribute* evAttrs) {
    // <META NAME=name CONTENT=val> <LINK REL=name HREF=val>
    // <META .....name........=val> <LINK ....name.....=val>
    int name = -1, val = 0;

    switch (ev.Tag->id()) {
        case HT_LINK:
            if ((name = FindAtt(TStringBuf("rev"), ev)) != -1) {
                const NHtml::TAttribute& nameAttr = evAttrs[name];

                if (nameAttr.Value.Leng && !strnicmp("canonical", ev.text + nameAttr.Value.Start,
                                                     nameAttr.Value.Leng)) {
                    return -1;
                }
            }

            if (((name = FindAtt(TStringBuf("rel"), ev)) != -1 || (name = FindAtt(TStringBuf("rev"), ev)) != -1) &&
                (val = FindAtt(TStringBuf("href"), ev)) != -1) {
                RenameAttribute(evAttrs[name], evAttrs[val]);
            }
            break;

        case HT_META:
            if (((name = FindAtt(TStringBuf("http-equiv"), ev)) != -1 ||
                 (name = FindAtt(TStringBuf("name"), ev)) != -1 ||
                 (name = FindAtt(TStringBuf("property"), ev)) != -1 ||
                 (name = FindAtt(TStringBuf("itemprop"), ev)) != -1)) {
                if ((val = FindAtt(TStringBuf("content"), ev)) != -1 ||
                    (val = FindAtt(TStringBuf("value"), ev)) != -1) {
                    RenameAttribute(evAttrs[name], evAttrs[val]);
                }
            }
            break;

        case HT_PARAM:
            if (((name = FindAtt(TStringBuf("name"), ev)) != -1) && (val = FindAtt(TStringBuf("value"), ev)) != -1) {
                RenameAttribute(evAttrs[name], evAttrs[val]);
            }
            break;

        default:
            break;
    }

    return name;
}

static inline int CheckLinkHreflang(const THtmlChunk& ev) {
    int val = -1;
    if (ev.Tag->id() == HT_LINK && (val = FindAtt(TStringBuf("hreflang"), ev)) != -1) {
        if (ev.Attrs[val].Value.Leng) {
            return val;
        }
    }
    return -1;
}

static inline THttpURL::TLinkType NormalizeLinkSetBase(const IParsedDocProperties& props, const char* link, THttpURL* base, THttpURL* result) {
    THttpURL::TLinkType ltype = NHtml::NormalizeLinkForSearch(THttpURL(), link, result, props.GetCharset());
    if (ltype != THttpURL::LinkIsGlobal) {
        if (!(result->GetFieldMask() & NUri::TField::FlagPath)) {
            return ltype;
        }
        const THttpURL& base2 = props.GetBase();
        NHtml::NormalizeLink(base2, result->Get(THttpURL::FieldPath), result, props.GetCharset(), props.GetCharset());
    }
    base->Copy(*result);
    base->Set(THttpURL::FieldQuery, nullptr);
    base->Set(THttpURL::FieldFragment, nullptr);
    if (props.GetBase().IsValidAbs())
        ltype = props.GetBase().Locality(*base);
    else
        ltype = THttpURL::LinkIsGlobal;
    return ltype;
}

static void FindSomeRelAttributes(const THtmlChunk& zoneEv, NHtml::TAttribute* attrs, bool& nofollow, bool& sponsored, bool& ugc) {
    nofollow = false;
    sponsored = false;
    ugc = false;

    for (unsigned i = 0; i < zoneEv.AttrCount; ++i) {
        const NHtml::TAttribute& a = attrs[i];

        if (a.Name.Leng != 3 || strnicmp(zoneEv.text + a.Name.Start, "rel", 3) != 0) {
            continue;
        }

        TVector<TStringBuf> values;
        StringSplitter(TStringBuf(zoneEv.text + a.Value.Start, a.Value.Leng)).SplitBySet(" \t,;").SkipEmpty().Collect(&values);
        for (auto& value : values) {
            if (value.size() == 8 && strnicmp(value.data(), "nofollow", 8) == 0) {
                nofollow = true;
            }
            if (value.size() == 9 && strnicmp(value.data(), "sponsored", 9) == 0) {
                sponsored = true;
            }
            if (value.size() == 3 && strnicmp(value.data(), "ugc", 3) == 0) {
                ugc = true;
            }
        }
    }
}

TAttributeExtractor::TAttributeExtractor(const THtConfigurator* config, bool collectLinks)
    : Configuration_(config)
    , CollectLinks_(collectLinks)
    , Follow_(true)
{
    Reset();
}

void TAttributeExtractor::Reset() {
    HtBase_.Clear();
}

void TAttributeExtractor::CheckEventImpl(const THtmlChunk& zoneEv, IParsedDocProperties* docProp, TZoneEntry* result) {
    // contract
    Y_ASSERT(result && !result->Name);
    Y_ASSERT(zoneEv.flags.type == PARSED_MARKUP && zoneEv.flags.markup != MARKUP_IGNORED);
    Y_ASSERT(zoneEv.GetLexType() == HTLEX_START_TAG || zoneEv.GetLexType() == HTLEX_EMPTY_TAG);
    Y_ASSERT(zoneEv.Tag);

    THtElemConf* wildElem = Configuration_->GetConf((size_t)HT_any);
    THtElemConf* elemConf = Configuration_->GetConf((size_t)zoneEv.Tag->id());
    if (!elemConf)
        elemConf = wildElem;
    if (!elemConf)
        return;

    bool is_zone = false;
    const char* pzone = nullptr; // pointer to 'zone' from configuration (zone name)

    // empty name of attribute means that the zone (and its name) is unconditional
    if (elemConf->CondAttrZones.size() == 1 && elemConf->CondAttrZones.count("")) {
        pzone = elemConf->CondAttrZones[""].data();
        is_zone = true;
    }

    if (!zoneEv.AttrCount) {
        if (is_zone) {
            result->Name = pzone;
            result->IsOpen = true;
        }
        return;
    }

    // meta-rename copy of attributes
    TTempArray<NHtml::TAttribute> attrCopy(zoneEv.AttrCount);
    NHtml::TAttribute* attrs = attrCopy.Data();
    memcpy(attrs, zoneEv.Attrs, sizeof(NHtml::TAttribute) * zoneEv.AttrCount);
    int name = CheckMetaRename(zoneEv, attrs);

    const TString* pname; // pointer to TString 'Name' from configuration (attr name)
    TCiString* pCondZone;

    TMemoryPool buf(64 * 1024);

    // eliminating of double attributes (use allocator)
    TSet<THtAttrConf*> foundAttribs;

    bool isNofollowLink = false;
    FindSomeRelAttributes(zoneEv, attrs, isNofollowLink, result->HasSponsoredAttr, result->HasUserGeneratedContentAttr);
    isNofollowLink = !Follow_ || isNofollowLink;

    TString attrValue;
    for (unsigned i = 0; i < zoneEv.AttrCount; ++i) {
        THtAttrExtConf* attrExtConf = nullptr;
        const NHtml::TAttribute& a = attrs[i];
        bool super_wild_attr = false;

        TStringType attrName(zoneEv.text + a.Name.Start, a.Name.Leng, buf);
        SoftStrip(attrName);
        if (!attrName.size())
            continue;
        ToLower(attrName.begin(), attrName.size()); // always convert name to lower case
        if (!FindInHash(elemConf->Attrs, attrName.c_str(), &attrExtConf)) {
            if (i == (unsigned)name && elemConf->wildAttrConf) {
                attrExtConf = elemConf->wildAttrConf;
            } else if (wildElem && FindInHash(wildElem->Attrs, attrName.c_str(), &attrExtConf)) {
                ; // skip
            } else if (elemConf->superWildAttrConf) {
                attrExtConf = elemConf->superWildAttrConf;
                super_wild_attr = true;
                // continue;
            } else
                continue;
        }

        attrValue.assign(zoneEv.text + a.Value.Start, a.Value.Leng);
        SoftStrip(attrValue);
        const char* ext = "";
        unsigned extlen = 0;
        THtAttrConf* attrConf = nullptr;

        // 1 find out ext, extlen if we need to
        find_extension(attrValue, ext, extlen);

        if (attrExtConf->HasValueMatch) {
            TStringType equals("=", attrValue.c_str(), buf);
            ToLower(equals.begin(), equals.size());
            FindInHash(attrExtConf->Ext, equals.c_str(), &attrConf);
        }
        if (!attrConf && extlen) {
            TStringType extension(ext, extlen, buf);
            ToLower(extension.begin(), extension.size());
            FindInHash(attrExtConf->Ext, extension.c_str(), &attrConf);
        }
        if (!attrConf && !FindInHash(attrExtConf->Ext, "", &attrConf))
            continue;

        pname = &attrConf->Name;
        pCondZone = &attrConf->YxCondZone;

        if (attrConf->Func) {
            attrValue = CallToParseFunc(attrConf->Func, attrValue.c_str(), docProp);
        }

        TString unicodeURL;
        TString originalURL;

        if (attrConf->Type == ATTR_URL) {
            if (a.Value.Start == a.Name.Start && a.Value.Leng == a.Name.Leng) { // in case of <a href>...</a>
                continue;
            }
            THttpURL normalizedLink;
            THttpURL::TLinkType ltype;
            if (*pname == "base")
                ltype = NormalizeLinkSetBase(*docProp, attrValue.data(), &HtBase_, &normalizedLink);
            else {
                const THttpURL* realbase = HtBase_.IsValidAbs() ? &HtBase_ : &docProp->GetBase();
                ltype = NHtml::NormalizeLinkForCrawl(*realbase, attrValue.data(), &normalizedLink, docProp->GetCharset());

                // Если ссылка указывает на базовую страницу, но текущая страница не является базовой, то
                // такую ссылку необходимо учесть.
                if (ltype == THttpURL::LinkIsFragment && realbase == &HtBase_) {
                    if (HtBase_.GetField(THttpURL::FieldPath) != docProp->GetBase().GetField(THttpURL::FieldPath)) {
                        ltype = THttpURL::LinkIsLocal;
                    }
                }
            }

            if (Configuration_->IsUriFilterOff())
                originalURL = attrValue;
            else if (attrConf->Ignore && ltype == THttpURL::LinkIsFragment) {
                // accept local links for properties
            } else if (ltype == THttpURL::LinkIsBad || ltype == THttpURL::LinkBadAbs || ltype == THttpURL::LinkIsFragment)
                continue;

            const TString tmp = normalizedLink.PrintS(THttpURL::FlagNoFrag);
            if (!tmp.empty()) {
                const THttpURL* realbase = HtBase_.IsValidAbs() ? &HtBase_ : &docProp->GetBase();
                NHtml::NormalizeLinkForSearch(*realbase, attrValue.data(), &normalizedLink, docProp->GetCharset());
                unicodeURL = normalizedLink.PrintS(THttpURL::FlagNoFrag);
            }
            attrValue = tmp;

            if (ltype == THttpURL::LinkIsLocal && attrConf->LocalName.size()) {
                pname = &attrConf->LocalName;
                pCondZone = &attrConf->YxLocalCondZone;
            }

            // it is URL; but not for insertion into links (i.e. not html-link)
            if (CollectLinks_ && (*pname == "base" || (strnicmp(pname->data(), "link", 4) == 0 && !isNofollowLink)))
                if (!!attrValue) {
                    docProp->SetProperty(PP_LINKS_HASH, attrValue.data());
                }
        }

        if (!attrValue && Configuration_->IsUriFilterOff()) {
            attrValue.resize(URL_MAX + 10);
            HtLinkDecode(originalURL.data(), attrValue.begin(), attrValue.capacity(), docProp->GetCharset());
            attrValue.resize(strlen(attrValue.data()));
        }

        if (!attrValue)
            continue;

        // conditional not-empty (good!) attribute
        if (!is_zone && elemConf->CondAttrZones.count(*pname)) {
            pzone = elemConf->CondAttrZones[*pname].data();
            is_zone = true;
        }

        //skip restricted attrs
        if (is_zone && !pCondZone->is_null() && (*pCondZone) != pzone)
            continue;

        // eliminating of double attributes - do we really need this?
        if (!super_wild_attr) {
            if (foundAttribs.count(attrConf))
                continue;
            foundAttribs.insert(attrConf);
        }

        const char* yxAttr = pname->data(); // from configuration
        if (attrConf->Name == ZONE_WILD) {
            yxAttr = attrName.c_str();
        }

        // check for nofollow in meta robots
        if (stricmp(yxAttr, PP_ROBOTS) == 0) {
            Follow_ = (attrValue.at(1) != '0');
        }

        // skip refresh and robots in w0 context
        // see ROBOT-2122
        if ((stricmp(yxAttr, PP_REFRESH) == 0 || stricmp(yxAttr, PP_ROBOTS) == 0) && zoneEv.flags.weight == WEIGHT_ZERO)
            return;

        // do not set property for BASE and CHARSET, they have special meaning
        if (attrConf->Pos == APOS_DOCUMENT && stricmp(yxAttr, PP_CHARSET) != 0 && stricmp(yxAttr, PP_BASE) != 0) {
            TString valueBuf(attrValue);
            if (stricmp(yxAttr, PP_ALTERNATE) == 0) {
                int lang = CheckLinkHreflang(zoneEv);
                if (lang != -1) {
                    valueBuf = NRelAlternate::ConcatenateLangHref(
                        TStringBuf(zoneEv.text + zoneEv.Attrs[lang].Value.Start, zoneEv.Attrs[lang].Value.Leng),
                        valueBuf);
                    docProp->SetProperty(yxAttr, valueBuf.data());
                }
            } else {
                docProp->SetProperty(yxAttr, valueBuf.data());
            }
        }

        if (attrConf->Ignore) // properties -- not attributes
            continue;

        ATTR_TYPE type = (isNofollowLink && attrConf->Type == ATTR_URL) ? ATTR_NOFOLLOW_URL : attrConf->Type;
        result->Attrs.push_back(TAttrEntry(yxAttr, attrValue, type, attrConf->Pos));

        if (!unicodeURL.empty()) {
            result->Attrs.push_back(TAttrEntry(yxAttr, unicodeURL, ATTR_UNICODE_URL, attrConf->Pos));
        }
    }

    if (!result->Attrs.empty() && !elemConf->AnyAttr.is_null()) {
        result->Attrs.push_back(TAttrEntry(elemConf->AnyAttr, "1", ATTR_BOOLEAN, APOS_ZONE));
    }

    if (is_zone) {
        result->Name = pzone;
        result->IsOpen = true;
    }
}

void TAttributeExtractor::CheckEvent(const THtmlChunk& evnt, IParsedDocProperties* docProp, TZoneEntry* result) {
    // contract
    Y_ASSERT(result && !result->Name);
    Y_ASSERT(evnt.flags.type == PARSED_MARKUP && evnt.flags.markup != MARKUP_IGNORED);
    Y_ASSERT(evnt.Tag);
    Y_ASSERT(docProp);

    if (evnt.GetLexType() == HTLEX_START_TAG || evnt.GetLexType() == HTLEX_EMPTY_TAG) {
        CheckEventImpl(evnt, docProp, result);
    }
}
