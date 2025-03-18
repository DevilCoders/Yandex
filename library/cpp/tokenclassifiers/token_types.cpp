#include "token_types.h"

namespace NTokenClassification {
    bool IsClassifiedToken(TTokenTypes tokenTypes) {
        return tokenTypes != 0;
    }

}
