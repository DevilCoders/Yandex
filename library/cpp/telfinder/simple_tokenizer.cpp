#include "simple_tokenizer.h"

#include <util/charset/unidata.h>
#include <utility>

TVector<TToken> TSimpleTokenizer::BuildTokens(const TUtf16String& text) {
    TVector<TToken> tokens;
    TWtringBuf textToken;
    TWtringBuf spaceToken;
    bool textWas = false;

    for (const wchar16* iterText = text.data(); iterText != text.data() + text.length(); ++iterText) {
        if (IsAlnum(*iterText)) {
            if (textWas) {
                textWas = false;
                tokens.push_back(TToken(textToken, spaceToken));

                textToken = TWtringBuf();
                spaceToken = TWtringBuf();
            }
            if (textToken.data() == nullptr) {
                textToken = TWtringBuf(iterText, 1);
            } else {
                textToken = TWtringBuf(textToken.data(), textToken.size() + 1);
            }
        } else {
            if (!textWas)
                textWas = true;

            if (spaceToken.data() == nullptr) {
                spaceToken = TWtringBuf(iterText, 1);
            } else {
                spaceToken = TWtringBuf(spaceToken.data(), spaceToken.size() + 1);
            }
        }
    }

    if (textToken.size() || spaceToken.size()) {
        tokens.push_back(TToken(textToken, spaceToken));
    }
    return tokens;
}
