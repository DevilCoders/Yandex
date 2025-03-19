#pragma once

/*
 *  Created on: Dec 6, 2011
 *      Author: albert@
 *
 * $Id$
 */


#include <library/cpp/uri/uri.h>

#include <library/cpp/charset/codepage.h>
#include <util/string/strip.h>

namespace Nydx {
namespace NUriNorm {

class TFlags
{
public:
    typedef TFlags TdSelf;
    typedef TFlags TdFlags;
    typedef unsigned long long TdMask;

public:
    enum EBit {
        EF_NONE,
        EF_NORM_URI,
        EF_NORM_HOST,
        EF_NORM_PATH,
        EF_NORM_QRY,
        EF_NORM_FRAG,
        EF_DECODE_HTML,
        EF_PARSE_DEFAULT,
        EF_PARSE_BARE,
        EF_PARSE_ROBOT,
        EF_URI_USE_RFC,
        EF_URI_ALLOW_ROOTLESS,
        EF_NORM_HOST_SPEC,
        EF_ENCODE_EXTENDED,
        EF_LOWER_URI,
        EF_REMOVE_SCHEME,
        EF_HOST_LOWER,
        EF_HOST_REMOVE_WWW,
        EF_HOST_ALLOW_IDN,
        EF_ENCODE_SPC_AS_PLUS, // not just query
        EF_PATH_LOWER,
        EF_PATH_REMOVE_TRAIL_SLASH,
        EF_PATH_APPEND_DIR_TRAIL_SLASH,
        EF_PATH_REMOVE_INDEX_PAGE,
        EF_QRY_LOWER,
        EF_QRY_DECODE_AMP,
        EF_QRY_ALT_SEMICOL,
        EF_QRY_NORM_DELIM,
        EF_QRY_SORT,
        EF_QRY_REMOVE_EMPTY,
        EF_QRY_REMOVE_DUPS,
        // http://code.google.com/web/ajaxcrawling/docs/specification.html
        EF_QRY_ESC_FRAG,
        EF_QRY_UNESC_FRAG,

        EF_MAX
    };

protected:
    TdMask Flags_;
    unsigned MaxUriLen_;
    mutable long ParseAllow_;
    mutable long ParseExtra_;
    ECharset CsURI_;

public:
    TFlags()
        : Flags_(DEFAULT_MASK)
        , MaxUriLen_(0)
        , ParseAllow_(0)
        , ParseExtra_(0)
        , CsURI_(CODES_UTF8)
    {
    }
    void SetDefaultFlags()
    {
        SetFlags(DEFAULT_MASK);
    }
    void SetDisableAll()
    {
        SetFlags(0);
    }
    void SetParseOnly()
    {
        SetFlags(PARSE_ONLY_MASK);
    }
    void SetMaxUriLen(unsigned val = 0)
    {
        MaxUriLen_ = val;
    }

protected:
    void SetFlags(TdMask flags)
    {
        Flags_ = flags;
        ParseAllow_ = 0;
    }
    TdMask GetFlags() const
    {
        return Flags_;
    }
    bool CheckFlag(TdMask flag) const
    {
        return CheckFlags(flag, flag);
    }
    bool CheckFlags(TdMask flags, TdMask expect) const
    {
        return expect == (GetFlags() & flags);
    }
    bool CheckFlagsPosNeg(TdMask flagsP, TdMask flagsN) const
    {
        return CheckFlags(flagsP | flagsN, flagsP);
    }

public:
    void AddFlagMask(TdMask fldmask, TdMask grpmask)
    {
        SubFlagMask(grpmask);
        AddFlagMask(fldmask & grpmask);
    }
    void AddFlagMask(TdMask mask)
    {
        SetFlags(GetFlags() | mask);
    }
    void SubFlagMask(TdMask mask)
    {
        SetFlags(GetFlags() & ~mask);
    }
    void SetFlagMask(TdMask mask, bool ok)
    {
        if (ok)
            AddFlagMask(mask);
        else
            SubFlagMask(mask);
    }
    bool GetFlagMask(TdMask mask) const
    {
        return CheckFlag(mask);
    }

public:

#define DECLARE_FLAG_NAME(bit) \
    bit ## _MASK

#define DECLARE_FLAG_IMPL(mtd, bit) \
    enum { DECLARE_FLAG_NAME(bit) = (TdMask(1) << bit) }; \
    void Disable ## mtd() { SubFlagMask(DECLARE_FLAG_NAME(bit)); } \
    bool Get ## mtd() const { return GetFlagMask(DECLARE_FLAG_NAME(bit)); } \
    void Set ## mtd(bool ok) { if (ok) Enable ## mtd(); else Disable ## mtd(); } \
    void SetForce ## mtd (bool ok) { if (ok) EnableForce ## mtd(); else Disable ## mtd(); } \
    void Unset ## mtd(bool ok) { Set ## mtd(!ok); } \
    void UnsetForce ## mtd (bool ok) { SetForce ## mtd(!ok); }

#define DECLARE_FLAG0(mtd, bit) \
    DECLARE_FLAG_IMPL(mtd, bit) \
    void Enable ## mtd() { AddFlagMask(DECLARE_FLAG_NAME(bit)); } \
    void EnableForce ## mtd() { Enable ## mtd(); }

#define DECLARE_FLAG(mtd, bit, mtd2) \
    DECLARE_FLAG_IMPL(mtd, bit) \
    void Enable ## mtd() { AddFlagMask(DECLARE_FLAG_NAME(bit)); } \
    void EnableForce ## mtd() { EnableForce ## mtd2(); Enable ## mtd(); }

#define DECLARE_GROUP_FLAG(mtd, bit, mtd2, grpmask) \
    DECLARE_FLAG_IMPL(mtd, bit) \
    void Enable ## mtd() { AddFlagMask(DECLARE_FLAG_NAME(bit), grpmask); } \
    void EnableForce ## mtd() { EnableForce ## mtd2(); Enable ## mtd(); }

    // uri
    DECLARE_FLAG0(NormalizeURLs, EF_NORM_URI)
    DECLARE_FLAG(DecodeHTML, EF_DECODE_HTML, NormalizeURLs)
    DECLARE_FLAG(LowercaseURL, EF_LOWER_URI, NormalizeURLs)
    DECLARE_FLAG(EncodeExtended, EF_ENCODE_EXTENDED, NormalizeURLs)
    DECLARE_FLAG(NormalizeHostSpecific, EF_NORM_HOST_SPEC, NormalizeURLs)

    // parsing flags
    DECLARE_FLAG(UseRFC, EF_URI_USE_RFC, NormalizeURLs)
    DECLARE_FLAG(AllowRootless, EF_URI_ALLOW_ROOTLESS, NormalizeURLs)
    DECLARE_GROUP_FLAG(ParseDefault, EF_PARSE_DEFAULT, NormalizeURLs, PARSE_GROUP_MASK)
    DECLARE_GROUP_FLAG(ParseBare, EF_PARSE_BARE, NormalizeURLs, PARSE_GROUP_MASK)
    DECLARE_GROUP_FLAG(ParseRobot, EF_PARSE_ROBOT, NormalizeURLs, PARSE_GROUP_MASK)

    // scheme
    DECLARE_FLAG(RemoveScheme, EF_REMOVE_SCHEME, NormalizeURLs)

    // host
    DECLARE_FLAG(NormalizeHost, EF_NORM_HOST, NormalizeURLs)
    DECLARE_FLAG(LowercaseHost, EF_HOST_LOWER, NormalizeHost)
    DECLARE_FLAG(RemoveWWW, EF_HOST_REMOVE_WWW, NormalizeHost)
    DECLARE_FLAG(HostAllowIDN, EF_HOST_ALLOW_IDN, NormalizeHost)

    // path
    DECLARE_FLAG(NormalizePath, EF_NORM_PATH, NormalizeURLs)
    DECLARE_FLAG(LowercasePath, EF_PATH_LOWER, NormalizePath)
    DECLARE_FLAG(EncodeSpcAsPlus, EF_ENCODE_SPC_AS_PLUS, NormalizePath)
    DECLARE_FLAG(PathRemoveIndexPage, EF_PATH_REMOVE_INDEX_PAGE, NormalizePath)
    DECLARE_GROUP_FLAG(PathRemoveTrailSlash, EF_PATH_REMOVE_TRAIL_SLASH, NormalizePath, TRAIL_SLASH_GROUP_MASK)
    DECLARE_GROUP_FLAG(PathAppendDirTrailSlash, EF_PATH_APPEND_DIR_TRAIL_SLASH, NormalizePath, TRAIL_SLASH_GROUP_MASK)

    // query
    DECLARE_FLAG(NormalizeQry, EF_NORM_QRY, NormalizeURLs)
    DECLARE_FLAG(LowercaseQry, EF_QRY_LOWER, NormalizeQry)
    DECLARE_GROUP_FLAG(QryEscFrag, EF_QRY_ESC_FRAG, NormalizeQry, ESCAPE_FRAGMENT_GROUP_MASK)
    DECLARE_GROUP_FLAG(QryUnescFrag, EF_QRY_UNESC_FRAG, NormalizeQry, ESCAPE_FRAGMENT_GROUP_MASK)
    DECLARE_FLAG(QryAltSemicol, EF_QRY_ALT_SEMICOL, NormalizeQry)
    DECLARE_FLAG(QrySort, EF_QRY_SORT, NormalizeQry)
    DECLARE_FLAG(QryNormDelim, EF_QRY_NORM_DELIM, NormalizeQry)
    DECLARE_FLAG(QryRemoveEmpty, EF_QRY_REMOVE_EMPTY, NormalizeQry)
    DECLARE_FLAG(QryRemoveDups, EF_QRY_REMOVE_DUPS, NormalizeQry)

    // fragment
    DECLARE_FLAG(NormalizeFrag, EF_NORM_FRAG, NormalizeURLs)

    enum {
        PARSE_GROUP_MASK = 0
            | DECLARE_FLAG_NAME(EF_PARSE_BARE)
            | DECLARE_FLAG_NAME(EF_PARSE_DEFAULT)
            | DECLARE_FLAG_NAME(EF_PARSE_ROBOT)
            ,

        ESCAPE_FRAGMENT_GROUP_MASK = 0
            | DECLARE_FLAG_NAME(EF_QRY_ESC_FRAG)
            | DECLARE_FLAG_NAME(EF_QRY_UNESC_FRAG)
            ,

        TRAIL_SLASH_GROUP_MASK = 0
            | DECLARE_FLAG_NAME(EF_PATH_REMOVE_TRAIL_SLASH)
            | DECLARE_FLAG_NAME(EF_PATH_APPEND_DIR_TRAIL_SLASH)
            ,

        PARSE_ONLY_MASK = 0
            | DECLARE_FLAG_NAME(EF_NORM_URI)
            | DECLARE_FLAG_NAME(EF_PARSE_DEFAULT)
            | DECLARE_FLAG_NAME(EF_NORM_HOST)
            ,

        DEFAULT_MASK = PARSE_ONLY_MASK
            | DECLARE_FLAG_NAME(EF_HOST_LOWER)
            | DECLARE_FLAG_NAME(EF_NORM_PATH)
            | DECLARE_FLAG_NAME(EF_NORM_QRY)
            | DECLARE_FLAG_NAME(EF_NORM_FRAG)
            | DECLARE_FLAG_NAME(EF_NORM_HOST_SPEC)
            | DECLARE_FLAG_NAME(EF_PATH_REMOVE_INDEX_PAGE)
            | DECLARE_FLAG_NAME(EF_QRY_ALT_SEMICOL)
            | DECLARE_FLAG_NAME(EF_QRY_SORT)
            | DECLARE_FLAG_NAME(EF_QRY_REMOVE_EMPTY)
            | DECLARE_FLAG_NAME(EF_QRY_REMOVE_DUPS)
            | DECLARE_FLAG_NAME(EF_QRY_NORM_DELIM)
            | DECLARE_FLAG_NAME(EF_QRY_UNESC_FRAG)
            ,
    };

#undef DECLARE_FLAG
#undef DECLARE_FLAG0
#undef DECLARE_FLAG_IMPL
#undef DECLARE_FLAG_NAME
#undef DECLARE_GROUP_FLAG

public:
    void DisableNormalizeEscFrag()
    {
        SubFlagMask(ESCAPE_FRAGMENT_GROUP_MASK);
    }

private:
    static ECharset GetCharset(const char* cs)
    {
        return CharsetByName(cs);
    }

#define DECLARE_CHARSET_FLD(mtd, fld) \
    ECharset Get ## mtd() const { return fld; } \
    void Set ## mtd(ECharset cs) { fld = cs; } \
    void SetStr ## mtd(const char* cs) { Set ## mtd(GetCharset(cs)); } \
    void Set ## mtd(const char* cs) { SetStr ## mtd(cs); }

public:
    DECLARE_CHARSET_FLD(CharsetURI, CsURI_)

#undef DECLARE_CHARSET_FLD

public:
    /**
     * Parses the URL.
     * - Trims leading whitespace.
     * - Optionally lowercase the whole string (if --lowurl); legally,
     *   not all parts of a URL can be, but some other methods in our codebase
     *   do that, so this can be utilized for better matching between URLs.
     * - Parse the string into a @c ::NUri::TUri.
     */
    ::NUri::TUri::EParsed Parse(::NUri::TUri &obj, TStringBuf uri) const
    {
        StripString(uri, uri);
        return uri.empty() ? ::NUri::TUri::ParsedEmpty : obj.ParseUri(uri, ParseFlags(), MaxUriLen_);
    }

    NUri::TParseFlags ParseFlags() const
    {
        if (0 == ParseAllow_)
            ParseFlagsImpl();
        return NUri::TParseFlags(ParseAllow_, ParseExtra_);
    }

protected:
    void ParseFlagsImpl() const;
};

}
}
