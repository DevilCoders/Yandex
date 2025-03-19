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
    //! ISO 3166-1 alpha-3 https://en.wikipedia.org/wiki/ISO_3166-1
    static constexpr TStringBuf MAPPING[] {
        TStringBuf("unknown"),
        TStringBuf("afg"),
        TStringBuf("alb"),
        TStringBuf("dza"),
        TStringBuf("and"),
        TStringBuf("ago"),
        TStringBuf("atg"),
        TStringBuf("arg"),
        TStringBuf("arm"),
        TStringBuf("abw"),
        TStringBuf("aus"),
        TStringBuf("aut"),
        TStringBuf("aze"),
        TStringBuf("bhs"),
        TStringBuf("bhr"),
        TStringBuf("bgd"),
        TStringBuf("brb"),
        TStringBuf("blr"),
        TStringBuf("bel"),
        TStringBuf("blz"),
        TStringBuf("ben"),
        TStringBuf("bmu"),
        TStringBuf("btn"),
        TStringBuf("bol"),
        TStringBuf("bih"),
        TStringBuf("bwa"),
        TStringBuf("bra"),
        TStringBuf("vgb"),
        TStringBuf("brn"),
        TStringBuf("bgr"),
        TStringBuf("bfa"),
        TStringBuf("bdi"),
        TStringBuf("khm"),
        TStringBuf("cmr"),
        TStringBuf("can"),
        TStringBuf("cpv"),
        TStringBuf("caf"),
        TStringBuf("tcd"),
        TStringBuf("chl"),
        TStringBuf("chn"),
        TStringBuf("cck"),
        TStringBuf("col"),
        TStringBuf("com"),
        TStringBuf("cog"),
        TStringBuf("cok"),
        TStringBuf("cri"),
        TStringBuf("civ"),
        TStringBuf("hrv"),
        TStringBuf("cub"),
        TStringBuf("cuw"),
        TStringBuf("cyp"),
        TStringBuf("cze"),
        TStringBuf("cod"),
        TStringBuf("dnk"),
        TStringBuf("dji"),
        TStringBuf("dma"),
        TStringBuf("dom"),
        TStringBuf("tls"),
        TStringBuf("ecu"),
        TStringBuf("egy"),
        TStringBuf("slv"),
        TStringBuf("gnq"),
        TStringBuf("eri"),
        TStringBuf("est"),
        TStringBuf("eth"),
        TStringBuf("flk"),
        TStringBuf("fji"),
        TStringBuf("fin"),
        TStringBuf("fra"),
        TStringBuf("guf"),
        TStringBuf("pyf"),
        TStringBuf("gab"),
        TStringBuf("gmb"),
        TStringBuf("geo"),
        TStringBuf("deu"),
        TStringBuf("gha"),
        TStringBuf("gib"),
        TStringBuf("grc"),
        TStringBuf("grl"),
        TStringBuf("grd"),
        TStringBuf("gum"),
        TStringBuf("gtm"),
        TStringBuf("gin"),
        TStringBuf("gnb"),
        TStringBuf("guy"),
        TStringBuf("hti"),
        TStringBuf("hnd"),
        TStringBuf("hun"),
        TStringBuf("isl"),
        TStringBuf("ind"),
        TStringBuf("idn"),
        TStringBuf("irn"),
        TStringBuf("irq"),
        TStringBuf("irl"),
        TStringBuf("isr"),
        TStringBuf("ita"),
        TStringBuf("jam"),
        TStringBuf("jpn"),
        TStringBuf("jor"),
        TStringBuf("kaz"),
        TStringBuf("ken"),
        TStringBuf("kir"),
        TStringBuf("kwt"),
        TStringBuf("kgz"),
        TStringBuf("lao"),
        TStringBuf("lva"),
        TStringBuf("lbn"),
        TStringBuf("lso"),
        TStringBuf("lbr"),
        TStringBuf("lby"),
        TStringBuf("lie"),
        TStringBuf("ltu"),
        TStringBuf("lux"),
        TStringBuf("mkd"),
        TStringBuf("mdg"),
        TStringBuf("mwi"),
        TStringBuf("mys"),
        TStringBuf("mdv"),
        TStringBuf("mli"),
        TStringBuf("mlt"),
        TStringBuf("mtq"),
        TStringBuf("mrt"),
        TStringBuf("mus"),
        TStringBuf("mex"),
        TStringBuf("mda"),
        TStringBuf("mco"),
        TStringBuf("mng"),
        TStringBuf("mne"),
        TStringBuf("msr"),
        TStringBuf("mar"),
        TStringBuf("moz"),
        TStringBuf("mmr"),
        TStringBuf("nam"),
        TStringBuf("nru"),
        TStringBuf("npl"),
        TStringBuf("nld"),
        TStringBuf("ncl"),
        TStringBuf("nzl"),
        TStringBuf("nic"),
        TStringBuf("ner"),
        TStringBuf("nga"),
        TStringBuf("niu"),
        TStringBuf("nfk"),
        TStringBuf("prk"),
        TStringBuf("nor"),
        TStringBuf("omn"),
        TStringBuf("pak"),
        TStringBuf("plw"),
        TStringBuf("pse"),
        TStringBuf("pan"),
        TStringBuf("png"),
        TStringBuf("pry"),
        TStringBuf("per"),
        TStringBuf("phl"),
        TStringBuf("pol"),
        TStringBuf("prt"),
        TStringBuf("pri"),
        TStringBuf("qat"),
        TStringBuf("rou"),
        TStringBuf("rus"),
        TStringBuf("rwa"),
        TStringBuf("esh"),
        TStringBuf("kna"),
        TStringBuf("lca"),
        TStringBuf("vct"),
        TStringBuf("wsm"),
        TStringBuf("smr"),
        TStringBuf("stp"),
        TStringBuf("sau"),
        TStringBuf("sen"),
        TStringBuf("srb"),
        TStringBuf("syc"),
        TStringBuf("sle"),
        TStringBuf("sgp"),
        TStringBuf("sxm"),
        TStringBuf("svk"),
        TStringBuf("svn"),
        TStringBuf("slb"),
        TStringBuf("som"),
        TStringBuf("zaf"),
        TStringBuf("kor"),
        TStringBuf("ssd"),
        TStringBuf("esp"),
        TStringBuf("lka"),
        TStringBuf("sdn"),
        TStringBuf("sur"),
        TStringBuf("swz"),
        TStringBuf("swe"),
        TStringBuf("che"),
        TStringBuf("syr"),
        TStringBuf("tjk"),
        TStringBuf("tza"),
        TStringBuf("tha"),
        TStringBuf("cym"),
        TStringBuf("fsm"),
        TStringBuf("mhl"),
        TStringBuf("vir"),
        TStringBuf("tgo"),
        TStringBuf("ton"),
        TStringBuf("tto"),
        TStringBuf("tun"),
        TStringBuf("tur"),
        TStringBuf("tkm"),
        TStringBuf("tca"),
        TStringBuf("tuv"),
        TStringBuf("uga"),
        TStringBuf("ukr"),
        TStringBuf("are"),
        TStringBuf("gbr"),
        TStringBuf("usa"),
        TStringBuf("ury"),
        TStringBuf("uzb"),
        TStringBuf("vut"),
        TStringBuf("vat"),
        TStringBuf("ven"),
        TStringBuf("vnm"),
        TStringBuf("yem"),
        TStringBuf("zmb"),
        TStringBuf("zwe")
    };

    static constexpr size_t MAPPING_SIZE = Y_ARRAY_SIZE(MAPPING);

    static_assert(
        NCountry::ECountry_ARRAYSIZE == MAPPING_SIZE,
        "You must define ISO 3166-1 alpha-3 code for each member of NCountry::ECountry"
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

TStringBuf NCountry::ToAlphaThreeCode(const ECountry country) {
    return Default<THelper>().Name(country);
}

NCountry::ECountry NCountry::FromAlphaThreeCode(const TStringBuf code) {
    return Default<THelper>().Enum(code);
}

bool NCountry::TryFromAlphaThreeCode(const TStringBuf code, ECountry& country) {
    return Default<THelper>().Enum(code, country);
}

