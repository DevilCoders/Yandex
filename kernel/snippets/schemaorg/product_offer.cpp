#include "product_offer.h"
#include "schemaorg_parse.h"

#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <util/string/reverse.h>

namespace NSchemaOrg {
    static bool IsEqualIgnoreCase(TWtringBuf s1, TWtringBuf s2) {
        if (s1.size() != s2.size()) {
            return false;
        }
        const wchar16* ptr1 = s1.data();
        const wchar16* ptr2 = s2.data();
        for (size_t i = 0; i < s1.size(); ++i) {
            if (ToLower(*ptr1) != ToLower(*ptr2)) {
                return false;
            }
            ++ptr1;
            ++ptr2;
        }
        return true;
    }

    static const TUtf16String STR_OUT_OF_STOCK[] = {
        u"outofstock",
        u"out of stock",
        u"out_of_stock",
        u"outstock",
        u"нет в наличии",
        u"нет на складе",
        u"товара нет в наличии",
    };
    static const TUtf16String STR_IN_STOCK[] = {
        u"instock",
        u"in stock",
        u"in_stock",
        u"в наличии",
        u"на складе",
        u"товар в наличии",
        u"есть в наличии на складе",
    };
    static const TUtf16String STR_PRE_ORDER[] = {
        u"preorder",
        u"под заказ",
    };

    bool TOffer::OfferIsNotAvailable() const {
        if (!Availability) {
            return false;
        }
        TWtringBuf availability = CutSchemaPrefix(Availability);
        for (auto& str : STR_OUT_OF_STOCK) {
            if (IsEqualIgnoreCase(availability, str)) {
                return true;
            }
        }
        return false;
    }

    EItemAvailability ParseAvailability(const TUtf16String& availability) {
        if (availability) {
            TWtringBuf cutAvailability = CutSchemaPrefix(availability);
            for (auto& str : STR_OUT_OF_STOCK) {
                if (IsEqualIgnoreCase(cutAvailability, str)) {
                    return AVAIL_OUT_OF_STOCK;
                }
            }
            for (auto& str : STR_IN_STOCK) {
                if (IsEqualIgnoreCase(cutAvailability, str)) {
                    return AVAIL_IN_STOCK;
                }
            }
            for (auto& str : STR_PRE_ORDER) {
                if (IsEqualIgnoreCase(cutAvailability, str)) {
                    return AVAIL_PRE_ORDER;
                }
            }
        }
        return AVAIL_UNKNOWN;
    }

    enum EPriceCurrency {
        NONE = 0,
        XRB = 1, // Russian or Belarusian ruble (internal use only)
        RUB = 2, // Russian ruble
        USD = 3, // United States dollar
        EUR = 4, // Euro
        UAH = 5, // Ukrainian hryvnia
        BYR = 6, // Belarusian ruble
        KZT = 7, // Kazakhstani tenge
        TRY = 8, // Turkish lira
    };

    static const struct {
        EPriceCurrency Code;
        TUtf16String String;
    } CURRENCIES[] = {
        {RUB, u"RUB"},
        {RUB, u"RUR"},
        {RUB, u"\u20BD"}, // Russian rouble sign
        {XRB, u"РУБЛЕЙ"},
        {XRB, u"РУБЛЯ"},
        {XRB, u"РУБЛЬ"},
        {XRB, u"РУБЛ"},
        {XRB, u"РУБ"},
        {XRB, u"PУБ"}, // The first letter is Latin
        {XRB, u"PYБ"}, // The first two letters are Latin
        {XRB, u"Р"},
        {UAH, u"UAH"},
        {UAH, u"ГРН"},
        {UAH, u"\u20B4"}, // Hryvnia sign
        {USD, u"USD"},
        {USD, u"US $"},
        {USD, u"US$"},
        {USD, u"$"},
        {TRY, u"TRY"},
        {TRY, u"TRL"},
        {TRY, u"TL"},
        {EUR, u"EUR"},
        {EUR, u"\u20AC"}, // Euro sign
        {BYR, u"BYR"},
        {KZT, u"KZT"},
        {KZT, u"ТГ"},
        {KZT, u"ТҢГ"},
    };

    static EPriceCurrency ParseCurrency(TWtringBuf str) {
        if (str) {
            if (str.back() == '.') {
                str.Chop(1);
            }
            for (auto& currency : CURRENCIES) {
                if (IsEqualIgnoreCase(str, currency.String)) {
                    return currency.Code;
                }
            }
        }
        return NONE;
    }

    static void TrimPunct(TWtringBuf& str) {
        while (str && (IsSpace(str[0]) || IsPunct(str[0]))) {
            str.Skip(1);
        }
        while (str && (IsSpace(str.back()) || IsPunct(str.back()))) {
            str.Chop(1);
        }
    }

    static const TUtf16String DIGITS = u"0123456789";
    static const struct {
        TUtf16String Text;
        bool IsLowPrice;
    } PREFIXES[] = {
        {u"цена", false},
        {u"от", true},
        {u"from", true},
    };

    static bool ParsePriceField(TWtringBuf str, TWtringBuf& price, EPriceCurrency& curCode, bool& isLowPrice) {
        price.Clear();
        curCode = NONE;
        isLowPrice = false;

        size_t firstDigit = str.find_first_of(DIGITS);
        if (firstDigit == TUtf16String::npos) {
            return false;
        }
        size_t lastDigit = str.find_last_of(DIGITS);
        price = str.SubStr(firstDigit, lastDigit - firstDigit + 1);

        TWtringBuf leftPart = str.Head(firstDigit);
        TrimPunct(leftPart);
        for (auto& prefix : PREFIXES) {
            if (IsEqualIgnoreCase(leftPart.Head(prefix.Text.size()), prefix.Text)) {
                leftPart.Skip(prefix.Text.size());
                TrimPunct(leftPart);
                if (prefix.IsLowPrice) {
                    isLowPrice = true;
                }
            }
        }
        TWtringBuf rightPart = str.Tail(lastDigit + 1);
        TrimPunct(rightPart);

        if (leftPart && rightPart) {
            return false;
        }
        if (!leftPart && !rightPart) {
            return true;
        }
        TWtringBuf curText = leftPart ? leftPart : rightPart;
        for (auto& currency : CURRENCIES) {
            if (IsEqualIgnoreCase(curText, currency.String)) {
                curCode = currency.Code;
                return true;
            }
        }
        return false;
    }

    static inline bool IsRuHost(TStringBuf host) {
        return host.EndsWith(".ru") || host.EndsWith(".xn--p1ai") /*.рф*/ || host.StartsWith("ru.");
    }

    static inline bool IsByHost(TStringBuf host) {
        return host.EndsWith(".by");
    }

    static EPriceCurrency ValidateCurrency(EPriceCurrency schemaCur, EPriceCurrency textCur, bool isRuHost, bool isByHost) {
        if (textCur != NONE && schemaCur != NONE) {
            if (textCur == XRB && (schemaCur == RUB || schemaCur == BYR)) {
                // it's ok
            } else if (textCur != schemaCur) {
                return NONE;
            }
        }
        EPriceCurrency cur = schemaCur != NONE ? schemaCur : textCur;
        if (cur == XRB) {
            if (isRuHost) {
                return RUB;
            } else if (isByHost) {
                return BYR;
            } else {
                return NONE;
            }
        }
        return cur;
    }

    static ui64 GetPriceInt(TWtringBuf price) {
        if (!price) {
            return 0;
        }
        ui64 result = 0;
        wchar16 decimalDot = 0;
        ssize_t pos = (ssize_t)price.length() - 1;
        if (price.length() >= 3) {
            wchar16 c1 = price[price.length() - 3];
            wchar16 c2 = price[price.length() - 2];
            wchar16 c3 = price.back();
            if ((c1 == '.' || c1 == ',') && c2 >= '0' && c2 <= '9' && c3 >= '0' && c3 <= '9') {
                decimalDot = c1;
                result = (ui64)(c2 - '0') * 10 + (ui64)(c3 - '0');
                pos = price.length() - 4;
            } else if ((c2 == '.' || c2 == ',') && c3 >= '0' && c3 <= '9') {
                decimalDot = c2;
                result = (ui64)(c3 - '0') * 10;
                pos = price.length() - 3;
            }
        }
        ui64 mul = 100;
        size_t lastSep = 0;
        wchar16 sep = 0;
        for (; pos >= 0; --pos) {
            wchar16 c = price[pos];
            if (c >= '0' && c <= '9') {
                result += mul * (ui64)(c - '0');
                mul *= 10;
                ++lastSep;
            } else if (lastSep == 3) {
                if (!sep && c != decimalDot && (c == ',' || c == '.' || c == '\'' || c == '`' || c == ' ')) {
                    sep = c;
                }
                if (sep && c == sep) {
                    lastSep = 0;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        }
        if (lastSep == 0 || price[0] == '0' && result >= 100) {
            return 0;
        }
        return result;
    }

    static TString FormatCurrencyCode(EPriceCurrency code) {
        switch (code) {
            case RUB:
                return "RUB";
            case USD:
                return "USD";
            case EUR:
                return "EUR";
            case UAH:
                return "UAH";
            case BYR:
                return "BYR";
            case KZT:
                return "KZT";
            case TRY:
                return "TRY";
            default:
                return TString();
        }
    }

    TPriceParsingResult ParsePrice(const TUtf16String& price, const TUtf16String& priceCurrency, TStringBuf host) {
        TPriceParsingResult result;
        result.IsValid = false;
        EPriceCurrency schemaCur = ParseCurrency(priceCurrency);
        if (priceCurrency && !schemaCur) {
            return result;
        }
        TWtringBuf priceBuf;
        EPriceCurrency textCur;
        bool isLowPrice;
        if (ParsePriceField(price, priceBuf, textCur, isLowPrice)) {
            bool isRuHost = IsRuHost(host);
            bool isByHost = IsByHost(host);
            EPriceCurrency curCode = ValidateCurrency(schemaCur, textCur, isRuHost, isByHost);
            if (curCode) {
                ui64 priceInt = GetPriceInt(ToWtring(priceBuf));
                if (priceInt > 0) {
                    result.IsValid = true;
                    result.IsLowPrice = isLowPrice;
                    result.ParsedPriceMul100 = priceInt;
                    result.FormattedPrice = ToWtring(priceBuf);
                    result.CurrencyCode = FormatCurrencyCode(curCode);
                    result.IsRuHost = isRuHost;
                    result.IsByHost = isByHost;
                    return result;
                }
            }
        }
        return result;
    }

    TPriceParsingResult TOffer::ParsePrice(TStringBuf host) const {
        TUtf16String price;
        if (LowPrice) {
            price = LowPrice;
        } else if (Price) {
            price = Price;
        }
        TPriceParsingResult result = NSchemaOrg::ParsePrice(price, PriceCurrency, host);
        if (result.IsValid && LowPrice) {
            result.IsLowPrice = true;
        }
        return result;
    }

    static TString FormatPriceValue(ui64 parsedPriceMul100) {
        ui64 num = parsedPriceMul100;
        bool needGroups = parsedPriceMul100 >= 1000000; // for numbers starting from 10 000.00
        TString str;
        if (num % 100) {
            ui64 frac = num % 100;
            str.append(static_cast<char>('0' + frac % 10));
            str.append(static_cast<char>('0' + frac / 10));
            str.append('.');
        }
        num /= 100;
        for (int i = 0; i == 0 || num; ++i) {
            ui64 dig = num % 10;
            if (needGroups && i > 0 && i % 3 == 0) {
                str.append(' ');
            }
            str.append(static_cast<char>('0' + dig));
            num /= 10;
        }
        ReverseInPlace(str);
        return str;
    }

    static const TUtf16String SPACE(wchar16(' '));
    static const TUtf16String EURO_SIGN(wchar16(0x20AC));
    static const TUtf16String RUS_RUB = u"рос. руб.";

    static const struct {
        TString Code;
        ELanguage Lang;
        TUtf16String Name;
    } PRICE_LOCALIZATION[] = {
        {"RUB", LANG_RUS, u"руб."},
        {"USD", LANG_RUS, u"$"},
        {"EUR", LANG_RUS, u"евро"},
        {"UAH", LANG_RUS, u"грн."},
        {"UAH", LANG_UKR, u"грн."},
        {"BYR", LANG_RUS, u"бел. руб."},
        {"BYR", LANG_BEL, u"бел. руб."},
        {"KZT", LANG_RUS, u"тнг"},
        {"KZT", LANG_KAZ, u"тңг"},
        {"TRY", LANG_TUR, u"TL"},
    };

    static TUtf16String GetPriceName(const TString& code, ELanguage lang, bool isByHost) {
        if (code == "RUB" && lang == LANG_RUS && isByHost) {
            return RUS_RUB;
        }
        for (const auto& loc : PRICE_LOCALIZATION) {
            if (code == loc.Code && lang == loc.Lang) {
                return loc.Name;
            }
        }
        if (code == "EUR") {
            return EURO_SIGN;
        }
        return UTF8ToWide(code);
    }

    TUtf16String TPriceParsingResult::FormatPrice(ELanguage lang) const {
        TUtf16String value = UTF8ToWide(FormatPriceValue(ParsedPriceMul100));
        TUtf16String priceName = GetPriceName(CurrencyCode, lang, IsByHost);
        TUtf16String res = value + SPACE + priceName;
        if (IsLowPrice) {
            if (lang == LANG_RUS) {
                res = u"От " + res;
            } else if (lang == LANG_ENG) {
                res = u"From " + res;
            } else if (lang == LANG_UKR) {
                res = u"Від " + res;
            } else if (lang == LANG_TUR) {
                res += u"'den";
            }
        }
        return res;
    }

    TString TPriceParsingResult::GetPriceIntegerPart() const {
        return ToString(ParsedPriceMul100 / 100);
    }

    TString TPriceParsingResult::GetPriceFractionalPart() const {
        int cents = static_cast<int>(ParsedPriceMul100 % 100);
        return ToString(cents / 10) + ToString(cents % 10);
    }

    TString TPriceParsingResult::GetCurrencyCodeWithRur() const {
        if (CurrencyCode == "RUB") {
            return "RUR";
        }
        return CurrencyCode;
    }

} // namespace NSchemaOrg
