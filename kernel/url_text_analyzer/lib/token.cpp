#include "token.h"

namespace NUta::NPrivate {
    void Tokenize(const TWtringBuf& src, IUrlTokenHandler* handler) {
        TNlpTokenizer nlpTokenizer(*handler);
        nlpTokenizer.Tokenize(src.data(), src.size());
    }
}