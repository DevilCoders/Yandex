#pragma once

#include "config_global.h"

#include <library/cpp/langs/langs.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

class THttpCookies;

namespace NAntiRobot {
    static const char* const WIZARD_QUERY_CLASSES[] = {
          "download"
        , "brandnames"
        , "disease"
        , "kak"
        , "moscow"
        , "oao"
        , "porno"
        , "travel"
    };

    static const size_t NUM_WIZARD_QUERY_CLASSES = Y_ARRAY_SIZE(WIZARD_QUERY_CLASSES);

    static const char* const QUERY_LANGS[] = {
          "rus_mixed"
        , "strict_foreign"
        , "non_words"
        , "person"
        , "url_cyr"
        , "url_lat"
        , "translit"
    };

    static const size_t NUM_QUERY_LANGS = Y_ARRAY_SIZE(QUERY_LANGS);

    static const ELanguage PERSON_LANGS[] = {
          LANG_UNK
        , LANG_RUS
        , LANG_ENG
    };

    static const size_t NUM_PERSON_LANGS = Y_ARRAY_SIZE(PERSON_LANGS);

    class TWizardFactorsCalculator {
        public:
            struct TSyntaxFactors {
                inline TSyntaxFactors() noexcept {
                    Zero(*this);
                }

                inline size_t Restrictions() const noexcept {
                    return UrlRestr + SiteRestr + HostRestr + DomainRestr + InUrlRestr + OtherRestr;
                }

                inline bool HaveRestr() const noexcept {
                    return (bool)Restrictions();
                }

                inline bool HaveSyntax() const noexcept {
                    return HasMiscOps || HasUserOp || HasNecessity || HasFormType;
                }

                bool HasMiscOps;
                bool HasUserOp;
                bool HasNecessity;
                bool HasFormType;
                bool HasQuotes;
                size_t UrlRestr;
                size_t SiteRestr;
                size_t HostRestr;
                size_t DomainRestr;
                size_t InUrlRestr;
                size_t OtherRestr;
                size_t RequestLength;
                size_t NumWords;
                bool SyntaxError;
            };

            struct TWizardFactors {
                float Commercial;
                ui32 PopularityLevel;
                bool IsNav;
                bool WizardCacheHit;
                int GeoCity;
                float GeoLocality;
                float PornoLevel;
                bool QueryClass[NUM_WIZARD_QUERY_CLASSES];
                bool QueryLang[NUM_QUERY_LANGS];
                bool PersonLang[NUM_PERSON_LANGS];

                inline TWizardFactors() noexcept {
                    Zero(*this);
                }
            };

            struct TValues: public TSyntaxFactors, public TWizardFactors {
                TValues()
                    : WizardError(false)
                    , IsUtf8(true)
                {
                }

                void Print(IOutputStream& out) const;

                bool WizardError;
                bool IsUtf8;
            };

            TWizardFactorsCalculator(bool useOnlyIps=false);
            ~TWizardFactorsCalculator();

            void CalcFactors(const TString& reqText, const THttpCookies& cookies,
                             TValues& values, const TStringBuf reqid) const;

        private:
            class TImpl;
            THolder<TImpl> Wizard;
    };
}
