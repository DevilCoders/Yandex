#pragma once

#include <library/cpp/yconf/conf.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

//! types of request attributes
enum EReqAttrType {
    RA_UNKNOWN,
    RA_ZONE,
    RA_ATTR_SPECIAL,
    RA_ATTR_LITERAL,    //!< literal attributes, the value of such attribute always starts from quote
    RA_ATTR_INTEGER,    //!< integer attributes, the value of such attribute does not start from quote
    RA_ATTR_URL,        //!< URL attributes, their values are processed during parsing
    RA_ATTR_LITERAL_URL,//!< URL attributes, but their values are not fuly processed during parsing (for compatibility)
    RA_ATTR_DATE,
    RA_ATTR_BOOLEAN
};

enum EReqAttrTarget {
    RA_TARG_UNDEF,
    RA_TARG_ZONE,
    RA_TARG_DOC,
    RA_TARG_DOCURL
};

    const wchar16 ZONE_STR[] = u"ZONE";
    const wchar16 ATTR_LITERAL_STR[] = u"ATTR_LITERAL";
    const wchar16 ATTR_INTEGER_STR[] = u"ATTR_INTEGER";
    const wchar16 ATTR_URL_STR[] = u"ATTR_URL";
    const wchar16 ATTR_DATE_STR[] = u"ATTR_DATE";
    const wchar16 ATTR_BOOLEAN_STR[] = u"ATTR_BOOLEAN";

    // zones
    const wchar16 TITLE_STR[] = u"title";
    const wchar16 ADDRESS_STR[] = u"address";
    const wchar16 ANCHOR_STR[] = u"anchor";
    const wchar16 ANCHORINT_STR[] = u"anchorint";
    const wchar16 ANCHORMUS_STR[] = u"anchormus";
    const wchar16 QUOTE_STR[] = u"quote";
    const wchar16 DEL_STR[] = u"del";
    const wchar16 INS_STR[] = u"ins";
    // fake attributes used in posfilter
    const wchar16 INTEXT_STR[] = u"intext";
    const wchar16 INLINK_STR[] = u"inlink";
    // FIO zones
    const wchar16 FIONAME_STR[] = u"fioname";
    const wchar16 FIOINNAME_STR[] = u"fioinname";
    const wchar16 FINAME_STR[] = u"finame";
    const wchar16 FIINNAME_STR[] = u"fiinname";
    const wchar16 FIINOINNAME_STR[] = u"fiinoinname";
    const wchar16 FIO_STR[] = u"fio";

    // literal attributes
    // Meta
    const wchar16 CHARSET_STR[] = u"charset";
    const wchar16 LANGUAGE_STR[] = u"language";
    const wchar16 ROBOTS_STR[] = u"robots";
    // Special links (file names)
    const wchar16 STYLE_STR[] = u"style";
    const wchar16 PROFILE_STR[] = u"profile";
    const wchar16 SCRIPT_STR[] = u"script";
    const wchar16 IMAGE_STR[] = u"image";
    const wchar16 APPLET_STR[] = u"applet";
    const wchar16 OBJECT_STR[] = u"object";
    const wchar16 ACTION_STR[] = u"action";
    // additional attributes
    const wchar16 DOMAIN_STR[] = u"domain";
    const wchar16 HOST_STR[] = u"host";
    const wchar16 HOSTIP_STR[] = u"hostip";
    const wchar16 LANG_STR[] = u"lang";
    const wchar16 MIME_STR[] = u"mime";
    const wchar16 RHOST_STR[] = u"rhost";
    const wchar16 UPATH_STR[] = u"upath";
    const wchar16 LUPATH_STR[] = u"lupath";
    const wchar16 RUPATH_STR[] = u"rupath";
    const wchar16 WHOIS_STR[] = u"whois";
    const wchar16 INURL_STR[] = u"inurl";
    const wchar16 SITE_STR[] = u"site";
    const wchar16 NV_STR[] = u"nv";
    const wchar16 EXACT_URL_STR[] = u"exact_url";
    const wchar16 NEVER_REACHABLE_FROM_MORDA_STR[] = u"never_reachable_from_morda";
    const wchar16 IS_TURBO_ECOM_STR[] = u"is_turbo_ecom";
    const wchar16 IS_CM2_STR[] = u"is_top_clicked_comm_host";
    const wchar16 IS_YAN_STR[] = u"is_yan_host";
    const wchar16 IS_FILTER_EXP1_STR[] = u"is_filter_exp1";
    const wchar16 IS_FILTER_EXP2_STR[] = u"is_filter_exp2";
    const wchar16 IS_FILTER_EXP3_STR[] = u"is_filter_exp3";
    const wchar16 IS_FILTER_EXP4_STR[] = u"is_filter_exp4";
    const wchar16 IS_FILTER_EXP5_STR[] = u"is_filter_exp5";

    // URL attributes
    // Meta
    const wchar16 BASE_STR[] = u"base";
    const wchar16 REFRESH_STR[] = u"refresh";
    // Links
    const wchar16 LINK_STR[] = u"link";
    const wchar16 LINKINT_STR[] = u"linkint";
    const wchar16 LINKMUS_STR[] = u"linkmus";
    //
    const wchar16 URL_STR[] = u"url";

    // date attributes
    const wchar16 DATE_STR[] = u"date";
    const wchar16 IDATE_STR[] = u"idate";
    const wchar16 DDATE_STR[] = u"ddate"; // dater date

    // boolean attributes
    const wchar16 CYRILLIC_STR[] = u"cyrillic";
    const wchar16 NEW_STR[] = u"new";

    // integer attributes
    const wchar16 CAT_STR[] = u"cat";
    const wchar16 SIZE_STR[] = u"size";
    const wchar16 FIO_LEN_STR[] = u"fio_len";

    const wchar16 PREVREQ_STR[] = u"prevreq";
    const wchar16 SOFTNESS_STR[] = u"softness";
    const wchar16 NGR_SOFTNESS_STR[] = u"zone_softness";
    const wchar16 PREFIX_STR[] = u"keyprefix";
    const wchar16 INPOS_STR[] = u"inpos";
    const wchar16 REFINE_FACTOR_STR[] = u"refinefactor";

    const char WHITESPACE[] = { ' ', '\t', '\r', 0 };

//! represents collection of known attribute names that can be used in a request
//! @note At the present time any attributes can be used in requests because the old syntax is supported.
//!       All constructions of request that are specified in new attribute style: name:value, name:"value" or name:(value)
//!       are covered by this class, for example: zones, special attributes and different kinds of attributes.
//!       Every type of request attributes has an appropriate TOperType type.
class TReqAttrList {
public:
    struct TAttrData {
        EReqAttrType Type;
        EReqAttrTarget Target;
        bool Template;
        ui8 NGrammBase;
    };

private:
    typedef TMap<TUtf16String, TAttrData, TGreater<TUtf16String>> TMapType;
    TMapType ExactMap;
    TMapType TemplateMap;

private:
    bool Add(const TUtf16String& name, EReqAttrType type, EReqAttrTarget target = RA_TARG_UNDEF, bool templ = false, ui8 nGrammBase = 0) {
        Y_ASSERT(!name.empty());
        Y_ASSERT(type != RA_UNKNOWN);
        const TAttrData data = {type, target, templ, nGrammBase};
        TMapType& map = templ ? TemplateMap : ExactMap;
        TMapType& otherMap = templ ? ExactMap : TemplateMap;
        if (otherMap.contains(name)) {
            return false;
        }
        return map.insert(TMapType::value_type(name, data)).second;
    }

    static void SetAttrData(TAttrData& data, EReqAttrType type, EReqAttrTarget target, bool templ = false) {
        data.Type = type;
        data.Target = target;
        data.Template = templ;
    }

    void AddSpecialAttrs();

    // Throws exception if one of the keys in the template map is a prefix of another.
    void CheckTemplateMapForPrefixes() const;
    bool DoAdd(const TUtf16String& name, const TString& data);

    static void ConstructByDefault(TReqAttrList& list);
    static void MergeAndOverrideMap(const TMapType& src, TMapType& dst);
    static void RemoveTrailingWhitespaces(TString& s);

    static inline bool IsWhitespace(const char *c) {
        return strchr(WHITESPACE, *c);
    }

public:
    static void ConstructFromConfig(TReqAttrList& list, TYandexConfig& config, const TYandexConfig::Section& section);
    static void ConstructFromString(TReqAttrList& list, const char* str);
    template <class T>
    static void ConstructFromArray(TReqAttrList& list, const T& attrs) {
        for (TStringBuf s : attrs){
            s = StripString(s, IsWhitespace);
            TStringBuf name, value;
            s.Split(':', name, value);
            name = StripStringRight(name, IsWhitespace);
            value = StripStringLeft(value, IsWhitespace);
            if (!name.empty() && !value.empty())
                list.Add(UTF8ToWide(name), TString(value));
        }
        list.AddSpecialAttrs();
    }

    //! constructs list with default set of attributes
    TReqAttrList();

    //! constructs list based on the configuration
    //! @param config         configuration object to report errors
    //! @param section        section containing request attributes
    //! @param ignoreDefault  don't load default request attributes
    //! @param allowOverride  override default request attributes. If allowOverride==false and the config attr overrides the default one yexception is thrown
    TReqAttrList(TYandexConfig& config, const TYandexConfig::Section& section, bool ignoreDefault = true, bool allowOverride = true);

    //! @param str     list of attributes, for example: "url:ATTR_URL,doc\ncat:ATTR_INTEGER,doc\ntitle:ZONE"
    //! @param ignoreDefault  don't load default request attributes
    //! @param allowOverride  override default request attributes. If allowOverride==false and the config attr overrides the default one yexception is thrown
    explicit TReqAttrList(const char* str, bool ignoreDefault = true, bool allowOverride = true);

    //! @param attrs    array of attributes, for example: ["url:ATTR_URL,doc","cat:ATTR_INTEGER,doc","title:ZONE"]
    //! @param ignoreDefault  don't load default request attributes
    //! @param allowOverride  override default request attributes. If allowOverride==false and the config attr overrides the default one yexception is thrown
    template <class T>
    explicit TReqAttrList(const T& attrs, bool ignoreDefault = true, bool allowOverride = true) {
        if (!ignoreDefault)
            ConstructByDefault(*this);

        if (allowOverride && !ignoreDefault) {
            TReqAttrList fromConfig(attrs, true);
            MergeAndOverride(fromConfig);
        } else {
            ConstructFromArray(*this, attrs);
        }
        CheckTemplateMapForPrefixes();
    }

    //! delete from ExactMap only
    void DeleteAttr(const TUtf16String& attrName) {
        ExactMap.erase(attrName);
    }

    //! looks through the collection of attributes and returns type
    //! @param nodeName     node name to be looked for the type
    //! @return @c RA_UNKNOWN if zone/attribute name is not found
    EReqAttrType GetType(const TUtf16String& attrName, ui32 processingFlag = 0) const;

    //! @todo probably it would be better to throw an exception if attrName is invalid
    const TAttrData* GetAttrData(const TUtf16String& attrName, ui32 processingFlag = 0) const;

    //! returns @c true if it is the zone type, not an attribute or a special attribute
    static bool IsZone(EReqAttrType type) {
        return (type == RA_ZONE);
    }

    //! returns @c true if it is an attribute type, not a zone or a special attribute
    static bool IsAttr(EReqAttrType type) {
        return (type == RA_ATTR_LITERAL || type == RA_ATTR_INTEGER || type == RA_ATTR_URL || type == RA_ATTR_LITERAL_URL || type == RA_ATTR_DATE || type == RA_ATTR_BOOLEAN);
    }

    bool IsUnknown(const wchar16* attrName) const {
        return GetType(attrName) == RA_UNKNOWN;
    }

    //! returns @c true if attribute is successfully added
    bool Add(const TUtf16String& name, const TString& data);

    void MergeAndOverride(const TReqAttrList& toMerge);
};
