#include "tokenizer.h"

#include <util/stream/output.h>

namespace NAddressFilter {

bool TTokenizer::IsCapital(const TWtringBuf& token) const {
    if (!IsUpper(token[0]))
        return false;
    return true;
}

bool TTokenizer::IsNumber(const NToken::TTokenInfo& token) const {
    return (token.SubTokens.size() == 1) && (token.SubTokens[0].Type == TOKEN_NUMBER);
}

TTokenType TTokenizer::GetTokenType(const NToken::TTokenInfo& tokenInfo, const TWtringBuf& token) const {
    TUtf16String lower(token);
    lower.to_lower();

    ui64 rawType = Trie.GetDefault(lower, 0);
    TTokenType tokenType(rawType);

    if (IsNumber(tokenInfo))
        tokenType.Set(TT_NUMBER);
    else
        tokenType.Set(TT_EMPTY);

    if (IsCapital(token))
        tokenType.Set(TT_CAPITAL);

    if (tokenType.Get(TT_CAPITAL) || tokenType.Get(TT_NUMBER)) {
        for(size_t i = TT_STREET_DICT_11S; i <= TT_STREET_DICT_44S; i++) {
            if (tokenType.Get(i))
                tokenType.Set(i - TT_STREET_DICT_11S + TT_STREET_DICT_11);
        }
    }

    return tokenType;
}

void TTokenizer::OnSentence(const NToken::TSentenceInfo& sentence) {
    TVector<TTokenType> tokenTypes;
    TVector<NToken::TTokenInfo> tokens;

    for (int i = sentence.Tokens.size() - 1; i >= 0; --i) {
            if (sentence.Tokens[i].IsNormalToken()) {
                TWtringBuf token(sentence.Text, sentence.Tokens[i].TokenOffset,  sentence.Tokens[i].Length);
                tokenTypes.push_back(GetTokenType(sentence.Tokens[i], token));
                tokens.push_back(sentence.Tokens[i]);
            } else {
                bool goodPunctuation = false;
                for(size_t j = 0; j < ALLOWED_PUNKTUATION_SIZE; j++) {
                    if (sentence.Tokens[i].Punctuation[0] == ALLOWED_PUNKTUATION[j]) {
                        goodPunctuation = true;
                        break;
                    }
                }
                if (!goodPunctuation) {
                    tokenTypes.push_back(TTokenType(0));
                    tokens.push_back(sentence.Tokens[i]);
                }
            }
        }

    Callback.OnText(sentence.Text, tokens, tokenTypes);
}

}
