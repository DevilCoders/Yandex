#include <util/charset/wide.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>

#include <contrib/libs/libidn/lib/idna.h>

#include "punycode_classifier.h"

namespace NTokenClassification {
    namespace NPunycodeClassification {
        using Pire::Encodings::Utf8;
        using Pire::Features::CaseInsensitive;
        using Pire::Lexer;
        using Pire::Runner;
        using Pire::Scanner;

        TPunycodeClassifier::TPunycodeClassifier()
            : PunycodeScanner(CompileRegexp())
        {
        }

        TPunycodeClassifier::~TPunycodeClassifier() {
        }

        ETokenType TPunycodeClassifier::Classify(const wchar16* tokenBegin,
                                                 size_t length) const {
            TString tokenUTF8 = WideToUTF8(tokenBegin, length);
            if (Matches(tokenUTF8.data(), tokenUTF8.end())) {
                return ETT_PUNYCODE;
            }

            return ETT_NONE;
        }

        void TPunycodeClassifier::GetMarkup(const wchar16* tokenBegin,
                                            size_t length,
                                            TMultitokenMarkupGroups& markupGroups) const {
            TString tokenUTF8 = WideToUTF8(tokenBegin, length);
            if (!Matches(tokenUTF8.begin(), tokenUTF8.end())) {
                ythrow yexception() << "Failed to match " + tokenUTF8 + " with url regexpr.";
            }

            markupGroups.AddMarkupGroup(EPMG_PUNYCODE, TInterval(tokenBegin, tokenBegin + length));
        }

        Scanner TPunycodeClassifier::CompileRegexp() {
            return Lexer(REG_EXP.begin(), REG_EXP.end())
                .AddFeature(CaseInsensitive())
                .SetEncoding(Utf8())
                .Parse()
                .Compile<Scanner>();
        }

        bool TPunycodeClassifier::Matches(const char* begin, const char* end) const {
            return Runner(PunycodeScanner)
                .Begin()
                .Run(begin, end)
                .End();
        }

        const TUtf16String TPunycodeClassifier::REG_EXP = u"^(xn--[-a-z0-9\\.]+)+$";

    }

}
