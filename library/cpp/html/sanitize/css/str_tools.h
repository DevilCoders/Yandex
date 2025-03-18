#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCssSanit {
    using TStrokaList = TVector<TString>;

    enum EResolveMode {
        ERM_NORMAL,
        ERM_URL
    };

    /** Return the string with translated all escape sequences
 * @param str       input string
 * @return          translated string
 */
    TString ResolveEscapes(const TString& str, EResolveMode mode = ERM_NORMAL);

    /** Encode url
 * @param str       string to encode
 * @return          encoded string
 */
    TString UrlEncode(const TString& str);

    TString UrlGetScheme(const TString& str);

    void GetStringList(const TString& class_list, TStrokaList& str_list);

    TString EscapeForbiddenUrlSymbols(const TString& str);

}
