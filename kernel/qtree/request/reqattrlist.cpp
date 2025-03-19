#include "reqattrlist.h"
#include <kernel/search_daemon_iface/reqtypes.h>
#include <util/generic/strbuf.h>
#include <util/string/vector.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace {

    inline EReqAttrType GetAttrType(TStringBuf s) {
        if (s == "ZONE"sv)
            return RA_ZONE;
        if (s == "ATTR_LITERAL"sv)
            return RA_ATTR_LITERAL;
        if (s == "ATTR_INTEGER"sv)
            return RA_ATTR_INTEGER;
        if (s == "ATTR_URL"sv)
            return RA_ATTR_URL;
        if (s == "ATTR_PRE_URL"sv)
            return RA_ATTR_LITERAL_URL;
        if (s == "ATTR_DATE"sv)
            return RA_ATTR_DATE;
        if (s == "ATTR_BOOLEAN"sv)
            return RA_ATTR_BOOLEAN;

        return RA_UNKNOWN;
    }

    inline EReqAttrTarget GetAttrTarget(TStringBuf s) {
        if (s == "zone"sv)
            return RA_TARG_ZONE;
        if (s == "doc")
            return RA_TARG_DOC;
        if (s == "docurl"sv)
            return RA_TARG_DOCURL;

        return RA_TARG_UNDEF;
    }
}

TReqAttrList::TReqAttrList() {
    ConstructByDefault(*this);
    CheckTemplateMapForPrefixes();
}

TReqAttrList::TReqAttrList(TYandexConfig& config, const TYandexConfig::Section& section, bool ignoreDefault, bool allowOverride) {
    if (!ignoreDefault)
        ConstructByDefault(*this);

    if (allowOverride && !ignoreDefault) {
        TReqAttrList fromConfig(config, section, true);
        MergeAndOverride(fromConfig);
    } else {
        ConstructFromConfig(*this, config, section);
    }
    CheckTemplateMapForPrefixes();
}

TReqAttrList::TReqAttrList(const char* str, bool ignoreDefault, bool allowOverride) {
    if (!ignoreDefault)
        ConstructByDefault(*this);

    if (allowOverride && !ignoreDefault) {
        TReqAttrList fromConfig(str, true);
        MergeAndOverride(fromConfig);
    } else {
        ConstructFromString(*this, str);
    }
    CheckTemplateMapForPrefixes();
}

EReqAttrType TReqAttrList::GetType(const TUtf16String& attrName, ui32 processingFlag) const {
    const TAttrData* data = GetAttrData(attrName, processingFlag);
    return data ? data->Type : RA_UNKNOWN;
}

const TReqAttrList::TAttrData* TReqAttrList::GetAttrData(const TUtf16String& attrName, ui32 processingFlag) const {
    if (!attrName)
        return nullptr;
    if ((processingFlag & RPF_ENABLE_EXTENDED_SYNTAX) == 0 &&
        attrName == TWtringBuf(REFINE_FACTOR_STR))
    {
        return nullptr;
    }

    auto exactPos = ExactMap.find(attrName);
    if (exactPos != ExactMap.end()) {
        return &exactPos->second;
    }
    auto templatePos = TemplateMap.lower_bound(attrName);
    if (templatePos != TemplateMap.end()) {
        const TUtf16String& key = templatePos->first;
        const TAttrData& data = templatePos->second;
        if (attrName.StartsWith(key))
            return &data;
    }
    return nullptr;
}

void TReqAttrList::MergeAndOverrideMap(const TMapType& src, TMapType& dst) {
    for (const auto& it : src)
        dst[it.first] = it.second;
}

void TReqAttrList::MergeAndOverride(const TReqAttrList& toMerge) {
    MergeAndOverrideMap(toMerge.ExactMap, ExactMap);
    MergeAndOverrideMap(toMerge.TemplateMap, TemplateMap);
    CheckTemplateMapForPrefixes();
}

void TReqAttrList::CheckTemplateMapForPrefixes() const {
    auto prev = TemplateMap.begin();
    if (prev == TemplateMap.end()) {
        return;
    }
    auto curr = prev;
    ++curr;
    while (curr != TemplateMap.end()) {
        // The map is sorted with TGreater,
        // so the prefix comes after the string.
        const TUtf16String& shorter = curr->first;
        const TUtf16String& longer = prev->first;
        if (longer.StartsWith(shorter)) {
            ythrow yexception() << "Bad template attributes: \"" << shorter << "\" is a prefix of \"" << longer << "\"";
        }
        ++prev;
        ++curr;
    }
}

bool TReqAttrList::Add(const TUtf16String& name, const TString& data) {
    const bool result = DoAdd(name, data);
    CheckTemplateMapForPrefixes();
    return result;
}

bool TReqAttrList::DoAdd(const TUtf16String& name, const TString& data) {
    if (name.empty())
        return false;

    // yandex.cfg example:
    // <Server>
    // </Server>
    // <Collection>
    //   <QueryLanguage>
    //      url    : ATTR_URL,doc
    //      cat    : ATTR_INTEGER,doc,,
    //      title  : ZONE
    //      anchor : ZONE,,,
    //      link   : ATTR_URL,zone
    //      upath  : ATTR_LITERAL,docurl
    //      search_literal  : ATTR_LITERAL,docurl,template
    //   </QueryLanguage>
    // </Collection>
    TVector<TStringBuf> parts;
    StringSplitter(data).Split(',').AddTo(&parts);
    for (auto& part : parts)
        StripString(part, part);
    const EReqAttrType type = !parts.empty() ? GetAttrType(parts[0]) : RA_UNKNOWN;
    if (type == RA_UNKNOWN)
        return false;
    const EReqAttrTarget target = parts.size() > 1 ? GetAttrTarget(parts[1]) : RA_TARG_UNDEF;
    if (target == RA_TARG_UNDEF && parts.size() > 1 && !! parts[1])
        return false;
    bool templ = parts.size() > 2 && parts[2] == "template";
    ui8 nGrammBase = 0;
    if (parts.size() > 3 && parts[3].StartsWith("ngr-")) {
        TryFromString(parts[3].substr(4), nGrammBase);
    }
    if (!Add(name, type, target, templ, nGrammBase)) {
        ythrow yexception() << "Cannot redefine attribute/zone: \"" << name << "\" is already defined";
    }
    return true;
}

void TReqAttrList::ConstructFromConfig(TReqAttrList& list, TYandexConfig& config, const TYandexConfig::Section& section) {
    auto it = section.GetDirectives().begin();
    for (; it != section.GetDirectives().end(); ++it) {
        Y_ASSERT(!it->first.empty());
        const TString s(JoinStrings(SplitString(it->second, " "), ""));
        try {
            if (!list.Add(UTF8ToWide(it->first), s)) {
                config.ReportError((it->first).data(), true, "type '%s' of the '%s' request attribute is unknown and will be ignored", it->second, it->first.c_str());
            }
        } catch (const yexception& e) {
            config.ReportError((it->first).data(), e.what(), false);
            throw;
        }
    }
    list.AddSpecialAttrs();
}

void TReqAttrList::RemoveTrailingWhitespaces(TString& s) {
    const char* b = s.c_str() - 1;
    const char* p = b + s.size();
    while (p != b && IsWhitespace(p))
        --p;
    s.resize(p - b);
}

void TReqAttrList::ConstructFromString(TReqAttrList& list, const char* str) {
    const char* p = str;
    TString name;
    TString value;
    while (*p) {
        const char* e = strchr(p, '\n');
        const size_t ws = strspn(p, WHITESPACE);
        if (e) {
            name.assign(p + ws, e);
            p = e + 1;
        } else {
            name.assign(p + ws);
            p += (ws + name.size());
        }
        const size_t delim = name.find(':');
        if (delim != TString::npos) {
            size_t n = delim + 1;
            n += strspn(name.c_str() + n, WHITESPACE);
            value.assign(name.c_str() + n, name.size() - n);
            RemoveTrailingWhitespaces(value);
            name.resize(delim);
            RemoveTrailingWhitespaces(name);
            if (!name.empty() && !value.empty())
                list.Add(UTF8ToWide(name), value);
        }
    }
    list.AddSpecialAttrs();
}

void TReqAttrList::ConstructByDefault(TReqAttrList& list) {
    // zones
    list.Add(TITLE_STR, RA_ZONE);
    list.Add(ADDRESS_STR, RA_ZONE);
    list.Add(ANCHOR_STR, RA_ZONE);
    list.Add(ANCHORINT_STR, RA_ZONE);
    list.Add(ANCHORMUS_STR, RA_ZONE);
    list.Add(QUOTE_STR, RA_ZONE);
    list.Add(DEL_STR, RA_ZONE);
    list.Add(INS_STR, RA_ZONE);
    // fake attributes used in posfilter
    list.Add(INTEXT_STR, RA_ZONE);
    list.Add(INLINK_STR, RA_ZONE);
    // FIO zones
    list.Add(FIONAME_STR, RA_ZONE);
    list.Add(FIOINNAME_STR, RA_ZONE);
    list.Add(FINAME_STR, RA_ZONE);
    list.Add(FIINNAME_STR, RA_ZONE);
    list.Add(FIINOINNAME_STR, RA_ZONE);
    list.Add(FIO_STR, RA_ZONE);


    list.Add(CHARSET_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(LANGUAGE_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(ROBOTS_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(STYLE_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(PROFILE_STR, RA_ATTR_LITERAL, RA_TARG_DOC);

    list.Add(HOST_STR, RA_ATTR_URL, RA_TARG_DOC);
    list.Add(HOSTIP_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(LANG_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(MIME_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(RHOST_STR, RA_ATTR_URL, RA_TARG_DOC);
    list.Add(WHOIS_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(INURL_STR, RA_ATTR_LITERAL_URL, RA_TARG_DOC); // @todo make it RA_ATTR_URL
    list.Add(SITE_STR, RA_ATTR_URL, RA_TARG_DOC);
    list.Add(NV_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(NEVER_REACHABLE_FROM_MORDA_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_TURBO_ECOM_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_CM2_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_YAN_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_FILTER_EXP1_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_FILTER_EXP2_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_FILTER_EXP3_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_FILTER_EXP4_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    list.Add(IS_FILTER_EXP5_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    // URL attributes
    list.Add(BASE_STR, RA_ATTR_URL, RA_TARG_DOC);
    list.Add(REFRESH_STR, RA_ATTR_URL, RA_TARG_DOC);
    list.Add(URL_STR, RA_ATTR_URL, RA_TARG_DOC);
    list.Add(EXACT_URL_STR, RA_ATTR_LITERAL, RA_TARG_DOC);
    // date attributes
    list.Add(DATE_STR, RA_ATTR_DATE, RA_TARG_DOC);
    list.Add(IDATE_STR, RA_ATTR_DATE, RA_TARG_DOC);
    // boolean attributes
    list.Add(CYRILLIC_STR, RA_ATTR_BOOLEAN, RA_TARG_DOC);
    list.Add(NEW_STR, RA_ATTR_BOOLEAN, RA_TARG_DOC);
    // integer attributes
    list.Add(CAT_STR, RA_ATTR_INTEGER, RA_TARG_DOC);
    list.Add(SIZE_STR, RA_ATTR_INTEGER, RA_TARG_DOC);


    list.Add(ACTION_STR, RA_ATTR_LITERAL, RA_TARG_ZONE);
    list.Add(APPLET_STR, RA_ATTR_LITERAL, RA_TARG_ZONE);
    list.Add(IMAGE_STR, RA_ATTR_LITERAL, RA_TARG_ZONE);
    list.Add(LINK_STR, RA_ATTR_URL, RA_TARG_ZONE);
    list.Add(LINKINT_STR, RA_ATTR_URL, RA_TARG_ZONE);
    list.Add(LINKMUS_STR, RA_ATTR_URL, RA_TARG_ZONE);
    list.Add(OBJECT_STR, RA_ATTR_LITERAL, RA_TARG_ZONE);
    list.Add(SCRIPT_STR, RA_ATTR_LITERAL, RA_TARG_ZONE);
    list.Add(DDATE_STR, RA_ATTR_LITERAL, RA_TARG_ZONE);
    list.Add(FIO_LEN_STR, RA_ATTR_INTEGER, RA_TARG_ZONE);

    list.Add(DOMAIN_STR, RA_ATTR_URL, RA_TARG_DOCURL);
    list.Add(UPATH_STR, RA_ATTR_LITERAL, RA_TARG_DOCURL);
    list.Add(LUPATH_STR, RA_ATTR_LITERAL, RA_TARG_DOCURL);
    list.Add(RUPATH_STR, RA_ATTR_LITERAL, RA_TARG_DOCURL);


    list.AddSpecialAttrs();
}

void TReqAttrList::AddSpecialAttrs() {
    // if attributes with such names were inserted they will be replaced
    SetAttrData(ExactMap[PREVREQ_STR], RA_ATTR_SPECIAL, RA_TARG_UNDEF);
    SetAttrData(ExactMap[SOFTNESS_STR], RA_ATTR_SPECIAL, RA_TARG_UNDEF);
    SetAttrData(ExactMap[PREFIX_STR], RA_ATTR_SPECIAL, RA_TARG_UNDEF);
    SetAttrData(ExactMap[NGR_SOFTNESS_STR], RA_ATTR_SPECIAL, RA_TARG_UNDEF);
    SetAttrData(ExactMap[INPOS_STR], RA_ATTR_SPECIAL, RA_TARG_UNDEF);
    SetAttrData(ExactMap[REFINE_FACTOR_STR], RA_ATTR_SPECIAL, RA_TARG_UNDEF);
}
