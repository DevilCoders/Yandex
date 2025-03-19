#include "titleproc.h"
#include <library/cpp/tokenizer/tokenizer.h>

TString TTitleHandler::GetTokenNormalizedTitleUTF8() const
{
    TString result;
    size_t maxSize = NormalizedTitleLengthLimit;
    for (const TUtf16String& titlePart : Title) {
        auto partRes = GetTokenNormalizedTitleUTF8Impl(titlePart, maxSize);

        if (!partRes.Result) {
            continue;
        }

        maxSize -= partRes.CodePointsResLength;
        result += partRes.Result;

        if (maxSize > 0 && !partRes.IsBorderReached) {
            result += ' ';
            maxSize -= 1;
        } else {
            break;
        }
    }

    if (!result.empty()) {
        if (result.back() == L' ') {
            result.pop_back();
        }
    }
    return result;
}


TTitleHandler::TPartTokenazationResult TTitleHandler::GetTokenNormalizedTitleUTF8Impl(
    TWtringBuf text, size_t maxLen)
{
    TPartTokenazationResult result;
    TUtf16String tmp;
    size_t leftSize = maxLen;

    auto callback = MakeCallbackTokenHandler([&](const TWideToken& token, size_t origleng, NLP_TYPE type) -> void {
        Y_UNUSED(origleng, type);
        if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT) {
            for (const TCharSpan * it = token.SubTokens.begin(); it != token.SubTokens.end(); ++it)
            {
                TWtringBuf subtoken(token.Token, it->Pos, it->Len);
                if (subtoken.size() >= leftSize) {
                    result.IsBorderReached = true;
                    break;
                } else {
                    tmp += subtoken;
                    leftSize -= subtoken.size();

                    if (leftSize > 0) {
                        tmp += L' ';
                        leftSize -= 1;
                    }
                }
            }
        }
    });
    TNlpTokenizer(callback, false).Tokenize(text, {});

    if (!tmp.empty()) {
        if (tmp.back() == L' ') {
            tmp.pop_back();
        }
    }
    tmp.to_lower();
    result.CodePointsResLength = tmp.size();

    Y_ASSERT(tmp.size() < maxLen);
    result.Result = WideToUTF8(tmp);
    return result;
}
