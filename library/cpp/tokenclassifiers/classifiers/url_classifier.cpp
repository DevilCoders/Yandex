#include <algorithm>

#include <util/charset/wide.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>

#include "url_classifier.h"

namespace NTokenClassification {
    namespace NUrlClassification {
        using std::find;
        using std::search;

        using Pire::Encodings::Utf8;
        using Pire::Features::CaseInsensitive;
        using Pire::Lexer;
        using Pire::Runner;
        using Pire::Scanner;

        TUrlClassifier::TUrlClassifier()
            : HostDelimiters(2)
            , PathDelimiters(2)
            , HostMarkupLayers(2)
            , PathMarkupLayers(2)
            , UrlScanner(CompileRegexp())
        {
            HostDelimiters[0].insert('.');
            HostDelimiters[1].insert('-');
            HostMarkupLayers[0] = EUPML_DOMAINS_TOKENS;
            HostMarkupLayers[1] = EUHML_ELEMENTARY_TOKENS;

            PathDelimiters[0].insert('/');
            PathDelimiters[1].insert('.');
            PathDelimiters[1].insert('-');
            PathDelimiters[1].insert('_');
            PathMarkupLayers[0] = EUPML_DIRECTORIES_TOKENS;
            PathMarkupLayers[1] = EUPML_ELEMENTARY_TOKENS;
        }

        TUrlClassifier::~TUrlClassifier() {
        }

        ETokenType TUrlClassifier::Classify(const wchar16* tokenBegin,
                                            size_t length) const {
            TString tokenUTF8 = WideToUTF8(tokenBegin, length);
            if (Matches(tokenUTF8.data(), tokenUTF8.end())) {
                return ETT_URL;
            }

            return ETT_NONE;
        }

        void TUrlClassifier::GetMarkup(const wchar16* tokenBegin,
                                       size_t length,
                                       TMultitokenMarkupGroups& markupGroups) const {
            TString tokenUTF8 = WideToUTF8(tokenBegin, length);
            if (!Matches(tokenUTF8.begin(), tokenUTF8.end())) {
                ythrow yexception() << "Failed to match " + tokenUTF8 + " with url regexpr.";
            }
            const wchar16* tokenEnd = tokenBegin + length;

            const wchar16* schemeEnd = search(tokenBegin, tokenEnd, SHCEME_DELIMITER.begin(), SHCEME_DELIMITER.end());
            const wchar16* hostBegin = tokenBegin;
            if (schemeEnd == tokenEnd) {
                schemeEnd = tokenBegin;
            } else {
                markupGroups.AddMarkupGroup(EUMG_SCHEME, TInterval(tokenBegin, schemeEnd));
                markupGroups.GetMarkupGroup(EUMG_SCHEME).AddMarkupLayer(EUSML_ELEMENTARY_TOKENS);
                markupGroups.GetMarkupGroup(EUMG_SCHEME).AddMarkupInterval(EUSML_ELEMENTARY_TOKENS, TInterval(tokenBegin, schemeEnd));
                hostBegin = schemeEnd + SHCEME_DELIMITER.length();
            }

            const wchar16* pathEnd = find(schemeEnd, tokenEnd, QUESTION_MARK);
            const wchar16* anchorStart = find(schemeEnd, tokenEnd, QUESTION_MARK);
            if (pathEnd > anchorStart) {
                pathEnd = anchorStart;
            }

            const wchar16* hostEnd = find(hostBegin, tokenEnd, SLASH);
            if (hostEnd == tokenEnd || pathEnd < hostEnd) {
                hostEnd = pathEnd;
            }

            markupGroups.AddMarkupGroup(EUMG_HOST, TInterval(hostBegin, hostEnd));
            FillGroupLayers(HostMarkupLayers, HostDelimiters, markupGroups.GetMarkupGroup(EUMG_HOST));

            markupGroups.AddMarkupGroup(EUMG_PATH, TInterval(hostEnd, pathEnd));
            FillGroupLayers(PathMarkupLayers, PathDelimiters, markupGroups.GetMarkupGroup(EUMG_PATH));
        }

        Scanner TUrlClassifier::CompileRegexp() {
            return Lexer(REG_EXP.begin(), REG_EXP.end())
                .AddFeature(CaseInsensitive())
                .SetEncoding(Utf8())
                .Parse()
                .Compile<Scanner>();
        }

        bool TUrlClassifier::Matches(const char* begin, const char* end) const {
            return Runner(UrlScanner)
                .Begin()
                .Run(begin, end)
                .End();
        }

        const TUtf16String TUrlClassifier::REG_EXP =
            u"^((http|ftp|https|file|фтп|хттп|шттп)://)?(www\\.|ввв\\.)?[a-zа-я0-9-\\.]+\\.(ru|ру|com|ком|ua|net|нет|biz|биз|info|инфо|org|орг|kz|edu|us|uk|рф)(/([a-zа-я0-9-\\.%/_]+)?)?([\?].+)?([#].+)?$";

        const wchar16 TUrlClassifier::SLASH = wchar16('/');
        const wchar16 TUrlClassifier::QUESTION_MARK = wchar16('?');
        const wchar16 TUrlClassifier::SHARP = wchar16('#');

        const TUtf16String TUrlClassifier::SHCEME_DELIMITER = u"://";

    }

}
