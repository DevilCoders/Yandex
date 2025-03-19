#include "software.h"
#include "product_offer.h"

#include <kernel/snippets/i18n/i18n.h>
#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/string/vector.h>

namespace NSchemaOrg {
    static const TUtf16String SPACE = u" ";
    static const TUtf16String DOT_SPACE = u". ";
    static const TUtf16String COLON_SPACE = u": ";
    static const TUtf16String PERIOD_SPACE = u", ";

    static const size_t MIN_FIELDS = 2;

    TUtf16String TSoftwareApplication::FormatSnip(TStringBuf host, ELanguage lang) const {
        if (!Description) {
            return TUtf16String();
        }
        TUtf16String snip;
        if (lang != LANG_UNK) {
            TVector<TUtf16String> fields;
            fields.reserve(5);
            fields.push_back(FormatPrice(host, lang));
            fields.push_back(FormatFileSize(host, lang));
            fields.push_back(FormatUserDownloads(lang));
            fields.push_back(FormatOperatingSystem());
            fields.push_back(FormatSubCategory(lang));
            size_t fieldsCount = CountIf(fields, [](const TUtf16String& s) { return !s.empty(); });
            if (fieldsCount >= MIN_FIELDS) {
                snip += JoinStrings(fields, TWtringBuf());
            }
        }
        snip += FormatDescription();
        return snip;
    }

    static const wchar16 BOM = wchar16(0xFEFF);
    static const wchar16 BLACK_STAR = wchar16(0x2605);

    TUtf16String TSoftwareApplication::FormatDescription() const {
        TUtf16String desc = Description;
        SubstGlobal(desc, TUtf16String(BOM), TUtf16String());
        SubstGlobal(desc, TUtf16String(BLACK_STAR), TUtf16String());
        Strip(desc);
        Collapse(desc);
        return desc;
    }

    static const TUtf16String ZERO_PRICE[] = {
        u"0",
        u"0.00",
        u"0,00",
        u"$0",
        u"$0.00",
        u"free",
        u"freeware",
        u"бесплатно",
    };

    TUtf16String TSoftwareApplication::FormatPrice(TStringBuf host, ELanguage lang) const {
        if (Price) {
            if (IsIn(ZERO_PRICE, ZERO_PRICE + Y_ARRAY_SIZE(ZERO_PRICE), to_lower(Price))) {
                return NSnippets::Localize("Free", lang) + DOT_SPACE;
            }
        }
        if (Price && PriceCurrency) {
            TPriceParsingResult res = ParsePrice(Price, PriceCurrency, host);
            if (res.IsValid) {
                TUtf16String price = res.FormatPrice(lang);
                if (price && price.back() != '.') {
                    price += '.';
                }
                price += ' ';
                return price;
            }
        }
        return TUtf16String();
    }

    static const TUtf16String USER_DOWNLOADS = u"UserDownloads:";
    static const TUtf16String NUM_MARKER = u"##";
    static constexpr TStringBuf MORE_THAN_BILLION = "More than ## billion downloads";
    static constexpr TStringBuf MORE_THAN_MILLION = "More than ## million downloads";
    static constexpr TStringBuf MORE_THAN_ONES = "More than ## downloads";
    static constexpr TStringBuf LESS_THAN_ONES = "Less than ## downloads";
    static const ui64 DOWNLOAD_COUNT_THOUSAND[] = {1, 3, 5, 10, 20, 30, 50, 100, 300, 500};
    static const ui64 DOWNLOAD_COUNT_MILLION[] = {5, 10, 50, 100, 200, 300, 400, 500};

    TUtf16String TSoftwareApplication::FormatUserDownloads(ELanguage lang) const {
        TWtringBuf wcount;
        for (const TUtf16String& str : InteractionCount) {
            if (TWtringBuf(str).AfterPrefix(USER_DOWNLOADS, wcount)) {
                break;
            }
        }
        if (!wcount) {
            return TUtf16String();
        }
        TString scount = WideToUTF8(wcount);
        SubstGlobal(scount, " ", TString());
        SubstGlobal(scount, ",", TString());
        ui64 count = 0;
        TryFromString<ui64>(scount, count);
        if (count == 0) {
            return TUtf16String();
        }

        TStringBuf templ;
        ui64 num = 0;
        if (count >= 1000 * 1000 * 1000) {
            templ = MORE_THAN_BILLION;
            num = 1;
        } else if (count >= 1000 * 1000) {
            templ = MORE_THAN_MILLION;
            num = 1;
            for (ui64 m : DOWNLOAD_COUNT_MILLION) {
                if (count > m * 1000 * 1000) {
                    num = m;
                }
            }
        } else if (count >= 100) {
            templ = MORE_THAN_ONES;
            num = 100;
            for (ui64 k : DOWNLOAD_COUNT_THOUSAND) {
                if (count > k * 1000) {
                    num = k * 1000;
                }
            }
        } else {
            templ = LESS_THAN_ONES;
            num = 100;
        }
        TUtf16String res;
        if (templ && num) {
            res = NSnippets::Localize(templ, lang);
            TString snum = ToString(num);
            if (num >= 10000) { // num < 1000 * 1000
                const char sep = lang == LANG_RUS ? ' ' : '.';
                snum.insert(snum.size() - 3, 1, sep);
            }
            SubstGlobal(res, NUM_MARKER, UTF8ToWide(snum));
            res += DOT_SPACE;
        }
        return res;
    }

    static const struct {
        TString Marker;
        int Flag;
    } OS_MARKERS[] = {
        {"windows phone", 1},
        {"win", 2},
        {"mac", 4},
        {"linux", 8},
        {"ios", 16},
        {"iphone", 16},
        {"ipad", 16},
        {"android", 32},
    };
    static const TUtf16String OS_NAMES[] = {
        u"Windows Phone",
        u"Windows",
        u"Mac OS",
        u"Linux",
        u"iOS",
        u"Android",
    };

    TUtf16String TSoftwareApplication::FormatOperatingSystem() const {
        if (!OperatingSystem) {
            return TUtf16String();
        }
        TString os;
        for (const TUtf16String& str : OperatingSystem) {
            os += WideToUTF8(str);
            os += ' ';
        }
        os.to_lower();
        int osMask = 0;
        for (const auto& marker : OS_MARKERS) {
            if (os.Contains(marker.Marker)) {
                osMask |= marker.Flag;
                SubstGlobal(os, marker.Marker, TString());
            }
        }
        if (osMask) {
            TUtf16String res;
            int flag = 1;
            for (const TUtf16String& name : OS_NAMES) {
                if (osMask & flag) {
                    if (res) {
                        res += PERIOD_SPACE;
                    }
                    res += name;
                }
                flag <<= 1;
            }
            if (res) {
                return res + DOT_SPACE;
            }
        }
        return TUtf16String();
    }

    static const size_t MAX_CATEGORY_LENGTH = 24;

    TUtf16String TSoftwareApplication::FormatSubCategory(ELanguage lang) const {
        TUtf16String res;
        for (const TUtf16String& cat : ApplicationSubCategory) {
            const size_t newLen = cat.size() + (res ? (res.size() + 2) : 0);
            if (newLen > MAX_CATEGORY_LENGTH) {
                break;
            }
            if (res) {
                res += PERIOD_SPACE;
            }
            res += cat;
        }
        if (res) {
            res = NSnippets::Localize("Category", lang) + COLON_SPACE + res + DOT_SPACE;
        }
        return res;
    }

    // Fixed unit for top sites
    static const struct {
        TString Host;
        int Mul;
    } HOST_UNITS[] = {
        {"www.tamindir.com", 1},
        {"www.gezginler.net", 1},
        {"oyun.tamindir.com", 1},
        {"soft.mydiv.net", 1024},
    };

    static const struct {
        TUtf16String Unit;
        int Mul;
    } FILE_SIZE_UNITS[] = {
        {u"k", 1024},
        {u"kb", 1024},
        {u"кб", 1024},
        {u"m", 1024 * 1024},
        {u"mb", 1024 * 1024},
        {u"мб", 1024 * 1024},
        {u"g", 1024 * 1024 * 1024},
        {u"gb", 1024 * 1024 * 1024},
        {u"гб", 1024 * 1024 * 1024},
    };

    TUtf16String TSoftwareApplication::FormatFileSize(TStringBuf host, ELanguage lang) const {
        if (!FileSize) {
            return TUtf16String();
        }
        TUtf16String fileSizeLower = FileSize;
        fileSizeLower.to_lower();
        TWtringBuf size(fileSizeLower);
        int mul = 0;
        for (const auto& unit : HOST_UNITS) {
            if (host == unit.Host) {
                mul = unit.Mul;
                break;
            }
        }
        if (mul == 0) {
            for (const auto& unit : FILE_SIZE_UNITS) {
                if (size.BeforeSuffix(unit.Unit, size)) {
                    mul = unit.Mul;
                    break;
                }
            }
        }
        if (mul > 0) {
            size.BeforeSuffix(SPACE, size);
            TString str = WideToUTF8(size);
            SubstGlobal(str, ',', '.');
            double num = 0.0;
            if (TryFromString<double>(str, num) && num > 0.0) {
                num = (num * mul) / (1024 * 1024); // convert to megabytes
                if (num < 0.1) {
                    num = 0.1;
                }
                TUtf16String unit;
                if (num < 1024.0) {
                    unit = NSnippets::Localize("MB", lang);
                } else {
                    num /= 1024;
                    unit = NSnippets::Localize("GB", lang);
                }
                TString snum = Sprintf(num < 9.95 ? "%.1lf" : "%.0lf", num);
                if (snum.EndsWith(".0")) {
                    snum.erase(snum.size() - 2);
                }
                if (lang == LANG_RUS) {
                    SubstGlobal(snum, '.', ',');
                }
                return NSnippets::Localize("Size", lang) + COLON_SPACE +
                       UTF8ToWide(snum) + SPACE + unit + DOT_SPACE;
            }
        }
        return TUtf16String();
    }

} // namespace NSchemaOrg
