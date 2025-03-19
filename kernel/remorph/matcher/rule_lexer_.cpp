#include "rule_lexer.h"

#include <util/generic/hash.h>

#define Y(TypeName, EnumName)                   \
    const TString& ToString(TypeName pt) {       \
        static struct Map {                     \
            THashMap<TypeName, TString> Data;   \
            Map() {                             \
                EnumName;                       \
            }                                   \
        } Map;                                  \
        return Map.Data[pt];                    \
    }

namespace NReMorph {

namespace NPrivate {

#define X(A) Data[A] = #A;
Y(TRuleLexerToken, RULE_LEXER_H_TOKENS_LIST)
#undef X

} // NPrivate

} // NReMorph
