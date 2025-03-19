
#include <kernel/search_types/search_types.h>
#include <util/string/escape.h>

#include "keyconv.h"


TFormToKeyConvertorWithCheck::TFormToKeyConvertorWithCheck()
{
    Zero(BadFirstChar);
    BadFirstChar[ui8(LEMMA_LANG_PREFIX)] = 1;
    BadFirstChar[ui8(ATTR_PREFIX)] = 1;
    BadFirstChar[ui8(OPEN_ZONE_PREFIX)] = 1;
    BadFirstChar[ui8(CLOSE_ZONE_PREFIX)] = 1;
}

bool TFormToKeyConvertorWithCheck::IsBadKey(TStringBuf key) const
{
    if (key.empty() || key.length() > MAXKEY_LEN) {
        return true;
    }
    if (BadFirstChar[ui8(key[0])]) {
        return true;
    }
    return false;
}

bool TFormToKeyConvertorWithCheck::IsBadQuery(TStringBuf query) const
{
    for (size_t i = 0; i < query.length(); ++i) {
        if (ui8(query[i]) < 32 || ui8(query[i]) == UTF8_FIRST_CHAR) {
            return true;
        }
    }
    return false;
}

const char* TFormToKeyConvertorWithCheck::ConvertUTF8(const char* token, size_t leng)
{
    if (IsBadQuery(TStringBuf(token, leng))) {
        ythrow yexception() << "Bad query [" << EscapeC(token, leng) << "]";
    }

    const char* indexKey = TFormToKeyConvertor::ConvertUTF8(token, leng);

    if (IsBadKey(indexKey)) {
        ythrow yexception() << "Bad key [" << EscapeC(indexKey, strlen(indexKey)) << "]";
    }
    return indexKey;
}


