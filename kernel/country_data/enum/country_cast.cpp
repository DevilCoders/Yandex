#include "iso.h"

#include <util/generic/cast.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

template <>
void Out<NCountry::ECountry>(IOutputStream& output, TTypeTraits<NCountry::ECountry>::TFuncParam x) {
    output.Write(NCountry::ToAlphaTwoCode(x));
}

template <>
NCountry::ECountry FromStringImpl(const char* data, size_t len) {
    NCountry::ECountry country;
    if (!TryFromString(data, len, country)) {
        ythrow TFromStringException{};
    }
    return country;
}

template <>
bool TryFromStringImpl(const char* data, size_t len, NCountry::ECountry& result) {
    return NCountry::TryFromAlphaTwoCode(TStringBuf{data, len}, result) ||
           NCountry::TryFromAlphaThreeCode(TStringBuf{data, len}, result);
}

