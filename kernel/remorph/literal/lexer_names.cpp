#include "lexer.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>

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

namespace NLiteral {

#define X(A) Data[A] = #A;
Y(TLiteralLexerToken, LITERAL_LEXER_H_TOKENS_LIST)
#undef X

} // NLiteral
