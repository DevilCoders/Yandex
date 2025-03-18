#pragma once

#include <util/generic/vector.h>

#include <library/cpp/regex/pire/pire.h>

#include <library/cpp/tokenclassifiers/token_classifier_interface.h>
#include <library/cpp/tokenclassifiers/token_types.h>
#include <library/cpp/tokenclassifiers/token_markup.h>

namespace NTokenClassification {
    namespace NEmailClassification {
        enum EEmailMarkupGroups {
            EEMG_LOGIN = 0,
            EEMG_DOMAIN = 1,
        };

        enum EEmailLoginMarkupLayers {
            EELML_ELEMENTARY_TOKENS = 0,
        };

        enum EEmailDomainMarkupLayers {
            EEDML_DOMAINS_TOKENS = 0,
            EEDML_ELEMENTARY_TOKENS = 1,
        };

        class TEmailClassifier: public ITokenClassifier {
        public:
            TEmailClassifier();
            ~TEmailClassifier() override;

            ETokenType Classify(const wchar16* tokenBegin, size_t length) const override;
            void GetMarkup(const wchar16* tokenBegin,
                           size_t length,
                           TMultitokenMarkupGroups& markupGroups) const override;

        private:
            typedef TVector<EEmailLoginMarkupLayers> TLoginMarkupLayers;
            typedef TVector<EEmailDomainMarkupLayers> TDomainMarkupLayers;

            static Pire::Scanner CompileRegexp();

            bool Matches(const char* begin, const char* end) const;

            static const TUtf16String REG_EXP;
            static const wchar16 AT_SING;

            TDelimeterMap LoginDelimiters;
            TDelimeterMap DomainDelimiters;

            TLoginMarkupLayers LoginMarkupLayers;
            TDomainMarkupLayers DomainMarkupLayers;

            Pire::Scanner EmailScanner;
        };

    }

}
