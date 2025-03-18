#pragma once

#include <util/generic/vector.h>

#include <library/cpp/regex/pire/pire.h>

#include <library/cpp/tokenclassifiers/token_classifier_interface.h>
#include <library/cpp/tokenclassifiers/token_types.h>
#include <library/cpp/tokenclassifiers/token_markup.h>

namespace NTokenClassification {
    namespace NUrlClassification {
        // Commented groups are currently not used.
        enum EUrlMarkupGroups {
            EUMG_SCHEME = 0,
            EUMG_HOST = 1,
            EUMG_PATH = 2,
            //EUMG_CGI      = 3,
            //EUMG_ANCHOR   = 4,
        };

        enum EUrlSchemeMarkupLayers {
            EUSML_ELEMENTARY_TOKENS = 0,
        };

        enum EUrlHostMarkupLayers {
            EUPML_DOMAINS_TOKENS = 0,
            EUHML_ELEMENTARY_TOKENS = 1,
        };

        enum EUrlPathMarkupLayers {
            EUPML_DIRECTORIES_TOKENS = 0,
            EUPML_ELEMENTARY_TOKENS = 1,
        };

        class TUrlClassifier: public ITokenClassifier {
        public:
            TUrlClassifier();
            ~TUrlClassifier() override;

            ETokenType Classify(const wchar16* tokenBegin, size_t length) const override;
            void GetMarkup(const wchar16* tokenBegin,
                           size_t length,
                           TMultitokenMarkupGroups& markupGroups) const override;

        private:
            typedef TVector<EUrlHostMarkupLayers> THostsMarkupLayers;
            typedef TVector<EUrlPathMarkupLayers> TPathMarkupLayers;

            static Pire::Scanner CompileRegexp();

            bool Matches(const char* begin, const char* end) const;

            static const TUtf16String REG_EXP;

            static const wchar16 SLASH;
            static const wchar16 QUESTION_MARK;
            static const wchar16 SHARP;

            static const TUtf16String SHCEME_DELIMITER;

            TDelimeterMap HostDelimiters;
            TDelimeterMap PathDelimiters;

            THostsMarkupLayers HostMarkupLayers;
            TPathMarkupLayers PathMarkupLayers;

            Pire::Scanner UrlScanner;
        };

    }

}
