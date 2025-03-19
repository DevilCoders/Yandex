#include "staticattr.h"

#include <util/charset/wide.h>

namespace NSnippets
{
    static const TStringBuf FIELD_DELIM = TStringBuf("\x07;"sv);
    static const char NAME_VALUE_DELIM = '=';

    static void ParseAttrs(TStringBuf infoValue, TStaticData::TAttrs& attrs)
    {
        while (infoValue) {
            TStringBuf field = infoValue.NextTok(FIELD_DELIM);
            TStringBuf name = field.NextTok(NAME_VALUE_DELIM);
            TStringBuf value = field;
            if (name && value) {
                attrs[name] = UTF8ToWide(value);
            }
        }
    }

    TStaticData::TStaticData(const TDocInfos& infos, const TString& infoName)
    {
        const TDocInfos::const_iterator it = infos.find(infoName.data());
        if (it != infos.end()) {
            Absent = false;
            ParseAttrs(it->second, Attrs);
        }
    }

    TStaticData::TStaticData(const TStringBuf& infoValue)
        : Absent(false)
    {
        ParseAttrs(infoValue, Attrs);
    }

    bool TStaticData::TryGetAttr(const TString& key, TUtf16String& dest) const {
        TAttrs::const_iterator i = Attrs.find(key);
        if (i == Attrs.end())
            return false;
        dest = i->second;
        return true;
    }
}

