#include "prod_preparer.h"

#include <library/cpp/charset/recyr.hh>
#include <util/charset/utf8.h>
#include <util/generic/vector.h>

namespace NStringMatchTracker {

    TString TProductionPreparer::Prepare(const TTextWithDescription& descr) const {
        TString res;
        const TString lowercase = ToLowerUTF8(descr.GetText());
        TStringBuf srcUtf8(lowercase.data(), lowercase.size());
        Recode(CODES_UTF8, CODES_YANDEX, srcUtf8, res);
        size_t p = 0;
        TVector<char> buf;
        buf.resize(res.size());
        for (size_t i = 0; i < res.size(); ++i) {
            if (IsGoodYandexLetter(res[i])) {
                buf[p] = res[i];
                ++p;
            }
        }
        res.assign(buf.data(), p);
        return res;
    }

    bool TProductionPreparer::IsGoodYandexLetter(unsigned char ch) const {
        if (ch >= 97 && ch <= 122) {//latin abc
            return true;
        } else if (ch == 184) {//yo
            return true;
        } else if (ch >= 224) {
            return true;
        }
        return false;
    }

}
