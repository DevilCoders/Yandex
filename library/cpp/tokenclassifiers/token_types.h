#pragma once

#include <util/system/defaults.h>

namespace NTokenClassification {
    enum ETokenType {
        ETT_NONE = 0,
        ETT_EMAIL = 1, // token type for unsplit email tokens - like maria@yandex.ru
        ETT_URL = 2,
        ETT_PUNYCODE = 4,
        ETT_SPLIT_EMAIL = 8, // token type marking split emails - the email token is no longer there, maria yandex ru
        ETT_URL_AND_EMAIL = ETT_EMAIL | ETT_URL,
        /*
   * Other types
   */
    };

    using TTokenTypes = ui32;

    bool IsClassifiedToken(TTokenTypes TokenTypes);

}
