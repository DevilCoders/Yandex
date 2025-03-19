#include "iso.h"

#include <util/generic/array_size.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/system/compiler.h>

#include <array>

namespace {
    //! ISO 3166-1 alpha-2 https://en.wikipedia.org/wiki/ISO_3166-1
    static constexpr TStringBuf MAPPING[] {
        TStringBuf("unknown"),
        TStringBuf("af"),
        TStringBuf("al"),
        TStringBuf("dz"),
        TStringBuf("ad"),
        TStringBuf("ao"),
        TStringBuf("ag"),
        TStringBuf("ar"),
        TStringBuf("am"),
        TStringBuf("aw"),
        TStringBuf("au"),
        TStringBuf("at"),
        TStringBuf("az"),
        TStringBuf("bs"),
        TStringBuf("bh"),
        TStringBuf("bd"),
        TStringBuf("bb"),
        TStringBuf("by"),
        TStringBuf("be"),
        TStringBuf("bz"),
        TStringBuf("bj"),
        TStringBuf("bm"),
        TStringBuf("bt"),
        TStringBuf("bo"),
        TStringBuf("ba"),
        TStringBuf("bw"),
        TStringBuf("br"),
        TStringBuf("vg"),
        TStringBuf("bn"),
        TStringBuf("bg"),
        TStringBuf("bf"),
        TStringBuf("bi"),
        TStringBuf("kh"),
        TStringBuf("cm"),
        TStringBuf("ca"),
        TStringBuf("cv"),
        TStringBuf("cf"),
        TStringBuf("td"),
        TStringBuf("cl"),
        TStringBuf("cn"),
        TStringBuf("cc"),
        TStringBuf("co"),
        TStringBuf("km"),
        TStringBuf("cg"),
        TStringBuf("ck"),
        TStringBuf("cr"),
        TStringBuf("ci"),
        TStringBuf("hr"),
        TStringBuf("cu"),
        TStringBuf("cw"),
        TStringBuf("cy"),
        TStringBuf("cz"),
        TStringBuf("cd"),
        TStringBuf("dk"),
        TStringBuf("dj"),
        TStringBuf("dm"),
        TStringBuf("do"),
        TStringBuf("tl"),
        TStringBuf("ec"),
        TStringBuf("eg"),
        TStringBuf("sv"),
        TStringBuf("gq"),
        TStringBuf("er"),
        TStringBuf("ee"),
        TStringBuf("et"),
        TStringBuf("fk"),
        TStringBuf("fj"),
        TStringBuf("fi"),
        TStringBuf("fr"),
        TStringBuf("gf"),
        TStringBuf("pf"),
        TStringBuf("ga"),
        TStringBuf("gm"),
        TStringBuf("ge"),
        TStringBuf("de"),
        TStringBuf("gh"),
        TStringBuf("gi"),
        TStringBuf("gr"),
        TStringBuf("gl"),
        TStringBuf("gd"),
        TStringBuf("gu"),
        TStringBuf("gt"),
        TStringBuf("gn"),
        TStringBuf("gw"),
        TStringBuf("gy"),
        TStringBuf("ht"),
        TStringBuf("hn"),
        TStringBuf("hu"),
        TStringBuf("is"),
        TStringBuf("in"),
        TStringBuf("id"),
        TStringBuf("ir"),
        TStringBuf("iq"),
        TStringBuf("ie"),
        TStringBuf("il"),
        TStringBuf("it"),
        TStringBuf("jm"),
        TStringBuf("jp"),
        TStringBuf("jo"),
        TStringBuf("kz"),
        TStringBuf("ke"),
        TStringBuf("ki"),
        TStringBuf("kw"),
        TStringBuf("kg"),
        TStringBuf("la"),
        TStringBuf("lv"),
        TStringBuf("lb"),
        TStringBuf("ls"),
        TStringBuf("lr"),
        TStringBuf("ly"),
        TStringBuf("li"),
        TStringBuf("lt"),
        TStringBuf("lu"),
        TStringBuf("mk"),
        TStringBuf("mg"),
        TStringBuf("mw"),
        TStringBuf("my"),
        TStringBuf("mv"),
        TStringBuf("ml"),
        TStringBuf("mt"),
        TStringBuf("mq"),
        TStringBuf("mr"),
        TStringBuf("mu"),
        TStringBuf("mx"),
        TStringBuf("md"),
        TStringBuf("mc"),
        TStringBuf("mn"),
        TStringBuf("me"),
        TStringBuf("ms"),
        TStringBuf("ma"),
        TStringBuf("mz"),
        TStringBuf("mm"),
        TStringBuf("na"),
        TStringBuf("nr"),
        TStringBuf("np"),
        TStringBuf("nl"),
        TStringBuf("nc"),
        TStringBuf("nz"),
        TStringBuf("ni"),
        TStringBuf("ne"),
        TStringBuf("ng"),
        TStringBuf("nu"),
        TStringBuf("nf"),
        TStringBuf("kp"),
        TStringBuf("no"),
        TStringBuf("om"),
        TStringBuf("pk"),
        TStringBuf("pw"),
        TStringBuf("ps"),
        TStringBuf("pa"),
        TStringBuf("pg"),
        TStringBuf("py"),
        TStringBuf("pe"),
        TStringBuf("ph"),
        TStringBuf("pl"),
        TStringBuf("pt"),
        TStringBuf("pr"),
        TStringBuf("qa"),
        TStringBuf("ro"),
        TStringBuf("ru"),
        TStringBuf("rw"),
        TStringBuf("eh"),
        TStringBuf("kn"),
        TStringBuf("lc"),
        TStringBuf("vc"),
        TStringBuf("ws"),
        TStringBuf("sm"),
        TStringBuf("st"),
        TStringBuf("sa"),
        TStringBuf("sn"),
        TStringBuf("rs"),
        TStringBuf("sc"),
        TStringBuf("sl"),
        TStringBuf("sg"),
        TStringBuf("sx"),
        TStringBuf("sk"),
        TStringBuf("si"),
        TStringBuf("sb"),
        TStringBuf("so"),
        TStringBuf("za"),
        TStringBuf("kr"),
        TStringBuf("ss"),
        TStringBuf("es"),
        TStringBuf("lk"),
        TStringBuf("sd"),
        TStringBuf("sr"),
        TStringBuf("sz"),
        TStringBuf("se"),
        TStringBuf("ch"),
        TStringBuf("sy"),
        TStringBuf("tj"),
        TStringBuf("tz"),
        TStringBuf("th"),
        TStringBuf("ky"),
        TStringBuf("fm"),
        TStringBuf("mh"),
        TStringBuf("vi"),
        TStringBuf("tg"),
        TStringBuf("to"),
        TStringBuf("tt"),
        TStringBuf("tn"),
        TStringBuf("tr"),
        TStringBuf("tm"),
        TStringBuf("tc"),
        TStringBuf("tv"),
        TStringBuf("ug"),
        TStringBuf("ua"),
        TStringBuf("ae"),
        TStringBuf("gb"),
        TStringBuf("us"),
        TStringBuf("uy"),
        TStringBuf("uz"),
        TStringBuf("vu"),
        TStringBuf("va"),
        TStringBuf("ve"),
        TStringBuf("vn"),
        TStringBuf("ye"),
        TStringBuf("zm"),
        TStringBuf("zw")
    };

    static constexpr size_t MAPPING_SIZE = Y_ARRAY_SIZE(MAPPING);

    static_assert(
        NCountry::ECountry_ARRAYSIZE == MAPPING_SIZE,
        "You must define ISO 3166-1 alpha-2 code for each member of NCountry::ECountry"
    );

    static_assert(
        0 == NCountry::ECountry_MIN,
        "Smallest value of NCountry::ECountry must be zero"
    );

    class THelper {
    public:
        THelper() {
            NameToEnum_.reserve(NCountry::ECountry_ARRAYSIZE);
            // expected that all values from 0 to ECountry_ARRAYSIZE - 1 are valid enums
            for (int country = 0; country < NCountry::ECountry_ARRAYSIZE; ++country) {
                EnumToName_[country] = MAPPING[country];
                if (NameToEnum_.contains(EnumToName_[country])) {
                    ythrow yexception() << '"' << EnumToName_[country]
                                        << "\" was mentioned at least twice";
                }

                NameToEnum_[EnumToName_[country]] = static_cast<NCountry::ECountry>(country);
            }
        }

        inline TStringBuf Name(const NCountry::ECountry country) const {
            if (Y_UNLIKELY(!NCountry::ECountry_IsValid(country))) {
                ythrow TFromStringException{};
            }

            return EnumToName_[country];
        }

        inline NCountry::ECountry Enum(const TStringBuf name) const {
            NCountry::ECountry country;
            if (Enum(name, country)) {
                return country;
            }

            ythrow TFromStringException{};
        }

        inline bool Enum(const TStringBuf name, NCountry::ECountry& country) const {
            if (const auto* const ptr = MapFindPtr(NameToEnum_, name)) {
                country = *ptr;
                return true;
            }

            return false;
        }

    private:
        THashMap<TStringBuf, NCountry::ECountry> NameToEnum_;
        std::array<TStringBuf, NCountry::ECountry_ARRAYSIZE> EnumToName_;
    };
}  // namespace

TStringBuf NCountry::ToAlphaTwoCode(const ECountry country) {
    return Default<THelper>().Name(country);
}

NCountry::ECountry NCountry::FromAlphaTwoCode(const TStringBuf code) {
    return Default<THelper>().Enum(code);
}

bool NCountry::TryFromAlphaTwoCode(const TStringBuf code, ECountry& country) {
    return Default<THelper>().Enum(code, country);
}

