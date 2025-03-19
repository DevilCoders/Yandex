#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NSchemaOrg {
    class TPriceParsingResult {
    public:
        bool IsValid;
        bool IsLowPrice;
        ui64 ParsedPriceMul100;
        TUtf16String FormattedPrice;
        TString CurrencyCode;
        bool IsRuHost;
        bool IsByHost;

        TPriceParsingResult()
            : IsValid(false)
            , IsLowPrice(false)
            , ParsedPriceMul100(0)
            , FormattedPrice()
            , CurrencyCode()
            , IsRuHost(false)
            , IsByHost(false)
        {
        }

        TUtf16String FormatPrice(ELanguage lang) const;
        TString GetPriceIntegerPart() const;
        TString GetPriceFractionalPart() const;
        TString GetCurrencyCodeWithRur() const;
    };

    enum EItemAvailability {
        AVAIL_UNKNOWN,
        AVAIL_IN_STOCK,
        AVAIL_OUT_OF_STOCK,
        AVAIL_PRE_ORDER,
    };
    EItemAvailability ParseAvailability(const TUtf16String& availability);

    TPriceParsingResult ParsePrice(const TUtf16String& price, const TUtf16String& priceCurrency, TStringBuf host);

    class TOffer {
    public:
        TUtf16String ProductName;
        TList<TUtf16String> ProductDesc;
        TList<TUtf16String> OfferDesc;
        TUtf16String Availability;
        TList<TUtf16String> AvailableAtOrFrom;
        TUtf16String ValidThrough;
        TUtf16String PriceValidUntil;
        TUtf16String Price;
        TUtf16String LowPrice;
        TUtf16String PriceCurrency;
        TString ErrorMessage;

        bool OfferIsNotAvailable() const;
        TPriceParsingResult ParsePrice(TStringBuf host) const;
    };
} // namespace NSchemaOrg
