#include <algorithm>

#include <util/charset/wide.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>

#include "email_classifier.h"

namespace NTokenClassification {
    namespace NEmailClassification {
        using std::back_inserter;
        using std::find;

        using Pire::Encodings::Utf8;
        using Pire::Features::CaseInsensitive;
        using Pire::Lexer;
        using Pire::Runner;
        using Pire::Scanner;

        TEmailClassifier::TEmailClassifier()
            : LoginDelimiters(1)
            , DomainDelimiters(2)
            , LoginMarkupLayers(1)
            , DomainMarkupLayers(2)
            , EmailScanner(CompileRegexp())
        {
            LoginDelimiters[0].insert('.');
            LoginDelimiters[0].insert('-');
            LoginDelimiters[0].insert('_');
            LoginMarkupLayers[0] = EELML_ELEMENTARY_TOKENS;

            DomainDelimiters[0].insert('.');
            DomainDelimiters[1].insert('-');
            DomainMarkupLayers[0] = EEDML_DOMAINS_TOKENS;
            DomainMarkupLayers[1] = EEDML_ELEMENTARY_TOKENS;
        }

        TEmailClassifier::~TEmailClassifier() {
        }

        ETokenType TEmailClassifier::Classify(const wchar16* tokenBegin,
                                              size_t length) const {
            TString tokenUTF8 = WideToUTF8(tokenBegin, length);
            if (Matches(tokenUTF8.data(), tokenUTF8.end())) {
                return ETT_EMAIL;
            }

            return ETT_NONE;
        }

        void TEmailClassifier::GetMarkup(const wchar16* tokenBegin,
                                         size_t length,
                                         TMultitokenMarkupGroups& markupGroups) const {
            TString tokenUTF8 = WideToUTF8(tokenBegin, length);
            if (!Matches(tokenUTF8.begin(), tokenUTF8.end())) {
                ythrow yexception() << "Failed to match " + tokenUTF8 + " with email regexpr.";
            }

            const wchar16* tokenEnd = tokenBegin + length;
            const wchar16* atSignPos = find(tokenBegin, tokenEnd, AT_SING);

            markupGroups.AddMarkupGroup(EEMG_LOGIN, TInterval(tokenBegin, atSignPos));
            FillGroupLayers(LoginMarkupLayers, LoginDelimiters, markupGroups.GetMarkupGroup(EEMG_LOGIN));
            markupGroups.AddMarkupGroup(EEMG_DOMAIN, TInterval(atSignPos + 1, tokenEnd));
            FillGroupLayers(DomainMarkupLayers, DomainDelimiters, markupGroups.GetMarkupGroup(EEMG_DOMAIN));
        }

        Scanner TEmailClassifier::CompileRegexp() {
            return Lexer(REG_EXP.begin(), REG_EXP.end())
                .AddFeature(CaseInsensitive())
                .SetEncoding(Utf8())
                .Parse()
                .Compile<Scanner>();
        }

        bool TEmailClassifier::Matches(const char* begin, const char* end) const {
            return Runner(EmailScanner)
                .Begin()
                .Run(begin, end)
                .End();
        }

        const TUtf16String TEmailClassifier::REG_EXP =
            u"^[a-zа-я0-9_\\.\\-]+@[a-zа-я0-9\\-]+(\\.[a-zа-я0-9\\-]+){1,3}$";

        const wchar16 TEmailClassifier::AT_SING = wchar16('@');

    }

}
