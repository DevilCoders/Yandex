#pragma once

#include <kernel/country_data/enum/protos/country.pb.h>

#include <util/generic/fwd.h>

// arcadia style cast via ToString/FromString is also available

namespace NCountry {
    //! ISO 3166-1 alpha-2 https://en.wikipedia.org/wiki/ISO_3166-1
    TStringBuf ToAlphaTwoCode(const ECountry country);
    //! ISO 3166-1 alpha-2 https://en.wikipedia.org/wiki/ISO_3166-1
    ECountry FromAlphaTwoCode(const TStringBuf code);
    //! ISO 3166-1 alpha-2 https://en.wikipedia.org/wiki/ISO_3166-1
    bool TryFromAlphaTwoCode(const TStringBuf code, ECountry& country);

    //! ISO 3166-1 alpha-3 https://en.wikipedia.org/wiki/ISO_3166-1
    TStringBuf ToAlphaThreeCode(const ECountry country);
    //! ISO 3166-1 alpha-3 https://en.wikipedia.org/wiki/ISO_3166-1
    ECountry FromAlphaThreeCode(const TStringBuf code);
    //! ISO 3166-1 alpha-3 https://en.wikipedia.org/wiki/ISO_3166-1
    bool TryFromAlphaThreeCode(const TStringBuf code, ECountry& country);
} // namespace NCountry
