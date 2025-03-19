#pragma once

#include <library/cpp/charset/recyr.hh>

#include <kernel/keyinv/invkeypos/keychars.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <util/string/cast.h>

// dst in utf-8
inline
void ConvertFromIndexKey(TStringBuf src, TString& dst)
{
    if (src.empty()) {
        dst.clear();
        return;
    } else if (src[0] == UTF8_FIRST_CHAR) {
        dst = ToString(src.SubStr(1));
    } else {
        dst = RecodeFromYandex(CODES_UTF8, ToString(src));
    }
}


class TFormToKeyConvertorWithCheck : private TFormToKeyConvertor
{
private:
    char BadFirstChar[256];

private:
    bool IsBadKey(TStringBuf key) const;
    bool IsBadQuery(TStringBuf query) const;

public:
    TFormToKeyConvertorWithCheck();

    // throws on errors
    const char* ConvertUTF8(const char* token, size_t leng); // leng in bytes, not characters
};

