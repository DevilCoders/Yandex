#pragma once

#include <util/generic/vector.h>

#include <library/cpp/regex/pire/pire.h>

#include <library/cpp/tokenclassifiers/token_classifier_interface.h>
#include <library/cpp/tokenclassifiers/token_types.h>
#include <library/cpp/tokenclassifiers/token_markup.h>

namespace NTokenClassification {
    namespace NPunycodeClassification {
        enum EPunycodeMarkupGroups {
            EPMG_PUNYCODE = 0,
        };

        class TPunycodeClassifier: public ITokenClassifier {
        public:
            TPunycodeClassifier();
            ~TPunycodeClassifier() override;

            ETokenType Classify(const wchar16* tokenBegin, size_t length) const override;
            void GetMarkup(const wchar16* tokenBegin,
                           size_t length,
                           TMultitokenMarkupGroups& markupGroups) const override;

        private:
            static Pire::Scanner CompileRegexp();

            bool Matches(const char* begin, const char* end) const;

            static const TUtf16String REG_EXP;

            Pire::Scanner PunycodeScanner;
        };

    }

}
