#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/stream/input.h>
#include <util/string/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

////
//  TAreaScheme
////

struct TAreaScheme {
    ui8 CountryOffset;
    ui8 CountryLength;
    ui8 AreaOffset;
    ui8 AreaLength;
    bool Weak;

    TAreaScheme();
    TAreaScheme(ui8 countryOffset, ui8 countryLength, ui8 areaOffset, ui8 areaLength, bool weak);
};

////
//  TPhoneSchemes

//  void ReadSchemes(IInputStream& in) - stream with phone templates.

//  Stream format must be:
//  phone_pattern countryOffset countryLength areaOffset areaLength weakness(optional)

//  Delimiter is tabular.
////

class TPhoneSchemes {
public:
    typedef TCompactTrie<char, TAreaScheme, TAsIsPacker<TAreaScheme>> TSchemeTrie;

private:
    TSchemeTrie Schemes;

    void InitDefaultSchemes();

public:
    TPhoneSchemes();
    TPhoneSchemes(const THashMap<TString, TAreaScheme>& schemes);

    void ReadSchemes(IInputStream& in);
    void Set(const THashMap<TString, TAreaScheme>& schemes);

    const TSchemeTrie& GetSchemes() const {
        return Schemes;
    }

    bool IsGood(const TStringBuf& pattern) const;
    bool TryGetScheme(const TStringBuf& pattern, TAreaScheme& scheme) const;
};
