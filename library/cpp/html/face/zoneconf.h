#pragma once

#include "parstypes.h"
#include "propface.h"
#include "serializedstring.h"

#include <library/cpp/mime/types/mime.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

struct THtmlChunk;

struct TAttrEntry {
    TSerializedString Name;
    TSerializedString Value;
    TWtringBuf DecodedValue;
    ATTR_TYPE Type;
    ATTR_POS Pos;

    TAttrEntry(const char* name, size_t nameLen, const char* value, size_t valueLen, ATTR_TYPE type = ATTR_LITERAL, ATTR_POS pos = APOS_ZONE)
        : Name(name, nameLen)
        , Value(value, valueLen)
        , DecodedValue(nullptr)
        , Type(type)
        , Pos(pos)
    {
    }

    TAttrEntry(const TString& name, const TString& val, ATTR_TYPE type = ATTR_LITERAL, ATTR_POS pos = APOS_ZONE)
        : Name(name)
        , Value(val)
        , DecodedValue(nullptr)
        , Type(type)
        , Pos(pos)
    {
    }
};

/// description of zone / attribute information related to markup entry
/// invariant: if (Name) assert(IsOpen || IsClose)
/// if IsOpen && IsClose, then zone is considered empty
/// if Name == 0, then info can still describe attributes with Pos == APOS_DOCUMENT
struct TZoneEntry {
    const char* Name;
    TVector<TAttrEntry> Attrs;
    bool IsOpen;
    bool IsClose;
    bool OnlyAttrs;
    bool NoOpeningTag;
    bool HasSponsoredAttr;
    bool HasUserGeneratedContentAttr;

    TZoneEntry()
        : Name(nullptr)
        , IsOpen(false)
        , IsClose(false)
        , OnlyAttrs(false)
        , NoOpeningTag(false)
        , HasSponsoredAttr(false)
        , HasUserGeneratedContentAttr(false)
    {
    }

    inline void Reset() {
        Name = nullptr;
        Attrs.clear();
        IsOpen = false;
        IsClose = false;
        OnlyAttrs = false;
        NoOpeningTag = false;
        HasSponsoredAttr = false;
        HasUserGeneratedContentAttr = false;
    }

    inline bool IsNofollow() const {
        for (size_t i = 0; i < Attrs.size(); ++i) {
            if (Attrs[i].Type == ATTR_NOFOLLOW_URL)
                return true;
        }
        return false;
    }

    inline bool IsSponsored() const {
        return HasSponsoredAttr;
    }

    inline bool IsUgc() const {
        return HasUserGeneratedContentAttr;
    }

    inline MimeTypes Mime() const {
        for (size_t i = 0; i < Attrs.size(); ++i) {
            const TAttrEntry& attr = Attrs[i];

            if (attr.Type == ATTR_LITERAL && attr.Pos == APOS_ZONE) {
                if (TCiString::compare(attr.Name.StrBuf(), TStringBuf("type")) == 0) {
                    return mimeByStr(attr.Value.c_str());
                }
            }
        }
        return MIME_UNKNOWN;
    }

    inline bool IsValid() const {
        return IsOpen || IsClose || !Attrs.empty();
    }
};

class IZoneAttrConf {
public:
    virtual ~IZoneAttrConf() {
    }
    virtual void CheckEvent(const THtmlChunk& evnt, IParsedDocProperties* docProp, TZoneEntry* result) = 0;
};
