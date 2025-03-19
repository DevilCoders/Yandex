#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <util/string/cast.h>

namespace NPrivate {

template <class TStringType>
struct TStringCnv {
    static TString ToUTF8(const TStringType& str);
    static TUtf16String ToWide(const TStringType& str);
};

template <>
struct TStringCnv<TString> {
    inline static const TString& ToUTF8(const TString& str) {
        return str;
    }

    inline static TUtf16String ToWide(const TString& str) {
        return UTF8ToWide(str);
    }
};

template <>
struct TStringCnv<TStringBuf> {
    inline static TString ToUTF8(const TStringBuf& str) {
        return ToString(str);
    }

    inline static TUtf16String ToWide(const TStringBuf& str) {
        return UTF8ToWide(str);
    }
};

template <>
struct TStringCnv<TUtf16String> {
    inline static TString ToUTF8(const TUtf16String& str) {
        return WideToUTF8(str);
    }

    inline static const TUtf16String& ToWide(const TUtf16String& str) {
        return str;
    }
};

template <>
struct TStringCnv<TWtringBuf> {
    inline static TString ToUTF8(const TWtringBuf& str) {
        return WideToUTF8(str);
    }

    inline static TUtf16String ToWide(const TWtringBuf& str) {
        return ToWtring(str);
    }
};

} // NPrivate

template <class TStringType>
inline TString ToUTF8(const TStringType& str) {
    return ::NPrivate::TStringCnv<TStringType>::ToUTF8(str);
}

template <class TStringType>
inline TUtf16String ToWide(const TStringType& str) {
    return ::NPrivate::TStringCnv<TStringType>::ToWide(str);
}
