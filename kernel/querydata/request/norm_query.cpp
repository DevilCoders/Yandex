#include "norm_query.h"

#include <ysite/yandex/doppelgangers/normalize.h>
#include <ysite/yandex/reqanalysis/normalize.h>
#include <kernel/searchlog/errorlog.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

namespace NQueryData {

    TString LowerCaseNormalization(TStringBuf userquery) {
        TUtf16String tmp = UTF8ToWide(userquery.data(), userquery.size());
        tmp.to_lower();
        Collapse(tmp);
        Strip(tmp);
        return WideToUTF8(tmp);
    }

    TString SimpleNormalization(TStringBuf userquery) {
        TUtf16String tmp = UTF8ToWide(userquery.data(), userquery.size());
        tmp.to_lower();
        TUtf16String res;
        for (TUtf16String::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
            if (IsAlnum(*it)) {
                wchar16 c = *it;
                if (L'\u0451' == c)      // ё
                    c = L'\u0435';       // е
                else if (L'\u00e7' == c) // ç
                    c = 'c';
                else if (L'\u011f' == c) // ğ
                    c = 'g';
                else if (L'\u0131' == c) // ı
                    c = 'i';
                else if (L'\u00f6' == c) // ö
                    c = 'o';
                else if (L'\u015f' == c) // ş
                    c = 's';
                else if (L'\u00fc' == c) // ü
                    c = 'u';
                res.append(c);
            } else
                res.append(' ');
        }
        Collapse(res);
        Strip(res);
        return WideToUTF8(res);
    }

    // TODO: make it language-aware
    TString DoppelNormalization(TStringBuf userquery) {
        return TDoppelgangersNormalize(true, false, false, false).NormalizeStd(TString{userquery});
    }

    TString DoppelNormalizationW(TStringBuf userquery) {
        return Default<NQueryNorm::TDoppWNormalizer>()(userquery);
    }

    TString NormalizeRequestUTF8Wrapper(TStringBuf uq) {
        return NormalizeRequestUTF8(TString{uq});
    }

    TStringBufs& GetOrMakeNormalization(TSubkeysCache& cache, EKeyType kt, TStringBuf reqfield, DoNormalizeQuery donormq,
                                        TStringBuf uquery) {
        TStringBufs& res = cache.GetSubkeysMutable(kt);

        if (res.empty()) {
            if (!!reqfield) {
                res.push_back(reqfield);
            } else {
                try {
                    PushBackPooled(res, cache.StringPool(), donormq(uquery));
                } catch (const yexception& e) {
                    SEARCH_ERROR << "Error while normalizing " << NEscJ::EscapeJ<true>(uquery) << " by "
                                 << EKeyType_Name(kt) << ": " << e.what();
                }
            }
        }

        return res;
    }

}
