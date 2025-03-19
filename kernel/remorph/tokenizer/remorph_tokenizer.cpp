#include "remorph_tokenizer.h"
#include "worker_pool.h"

#include <kernel/remorph/common/verbose.h>

#include <library/cpp/token/token_util.h>
#include <library/cpp/langmask/langmask.h>

#include <util/stream/trace.h>
#include <util/string/strip.h>
#include <util/string/split.h>
#include <util/string/type.h>
#include <util/system/guard.h>
#include <util/charset/wide.h>
#include <util/charset/unidata.h>

namespace NToken {

void TTokenizer::OnToken(const TWideToken& tok, size_t /*len*/, NLP_TYPE type) {
    switch (type) {
    case NLP_END:
    case NLP_SENTBREAK:
    case NLP_PARABREAK:
        if (tok.Leng > 0) {
            OnSentenceEnd(tok);
        }
        if (!Opts.Has(TF_NO_SENTENCE_SPLIT)) {
            FlushTokens();
        }
        break;
    case NLP_MISCTEXT:
        if (tok.Leng > 0) {
            OnMiscToken(tok);
        }
        break;
    default:
        OnNormalToken(tok);
        break;
    }
}

void TTokenizer::OnMiscToken(const TWideToken& tok) {
    if (!::IsSpace(tok.Token, tok.Leng)) {
        TUtf16String punct(tok.Token, tok.Leng);
        PutDelims(punct);
    } else {
        CurSentInfo.Pos.second += tok.Leng;
        CurSentInfo.Text.append(tok.Token, tok.Leng);
    }
    SpaceBefore = SpaceBefore || (tok.Leng && ::IsAsciiSpace(tok.Token[tok.Leng - 1]));
}

// NLP tokenizer parses sequential delimiters and spaces as the single wide token.
// Split such token to a separate tokens (one token per delimiter).
void TTokenizer::PutDelims(const TUtf16String& punct) {
    for (size_t s = 0; s < punct.length(); ++s) {
        if (!IsSpace(punct[s])) {
            PutToken(TUtf16String(punct.data(), s, 1));
        } else {
            ++CurSentInfo.Pos.second;
            CurSentInfo.Text.append(punct[s]);
            SpaceBefore = true;
        }
    }
}

void TTokenizer::PutToken(const TWideToken& token) {
    if (MS_ALL == Opts.MultitokenSplit && (token.SubTokens[0].PrefixLen || token.SubTokens.back().SuffixLen)) {
        TWideToken newToken = token;
        TUtf16String prefix;
        TUtf16String suffix;
        if (token.SubTokens[0].PrefixLen) {
            prefix = RemoveWideTokenPrefix(newToken);
            PutToken(prefix);
        }
        if (token.SubTokens.back().SuffixLen) {
            suffix = RemoveWideTokenSuffix(newToken);
        }
        CurSentInfo.Tokens.emplace_back();
        CurSentInfo.Tokens.back().Fill(CurSentInfo.Text.size(), GetCurrentSentencePos(), SpaceBefore, newToken);
        CurSentInfo.Pos.second += newToken.Leng;
        CurSentInfo.Text.append(newToken.Token, newToken.Leng);
        SpaceBefore = false;
        CheckLimit();
        if (!suffix.empty()) {
            PutToken(suffix);
        }
    } else {
        CurSentInfo.Tokens.emplace_back();
        CurSentInfo.Tokens.back().Fill(CurSentInfo.Text.size(), GetCurrentSentencePos(), SpaceBefore, token);
        CurSentInfo.Pos.second += token.Leng;
        CurSentInfo.Text.append(token.Token, token.Leng);
        SpaceBefore = false;
        CheckLimit();
    }
}

void TTokenizer::OnSentenceEnd(const TWideToken& tok) {
    if (!::IsSpace(tok.Token, tok.Leng)) {
        OnMiscToken(tok);
    } else {
        SpaceBefore = true;
    }
}

void TTokenizer::OnNormalToken(const TWideToken& tok) {
    Y_ASSERT(!tok.SubTokens.empty() && (tok.SubTokens.size() != 1 || tok.SubTokens[0].Len != 0));

    // Single token.
    if (tok.SubTokens.size() == 1) {
        PutToken(tok);
        return;
    }

    // Multitoken.
    size_t iStart = 0, i = 0;
    for (; i < tok.SubTokens.size() - 1; ++i) {
        bool split = false;
        switch (Opts.MultitokenSplit) {
        case MS_MINIMAL:
            // In minimal mode split only by dot and only in special cases.
            split = CheckWideTokenDotSplit(tok, i);
            break;
        case MS_SMART:
            // In smart mode split all multitokens, except those separated by hyphen or apostrophe.
            // Also split multitokens with subtokens of different types and groups of numbers.
            split = CheckWideTokenSplit(tok, i);
            break;
        case MS_ALL:
            // Split all multitokens.
            split = true;
            break;
        }

        if (split) {
            AddSubtokensRange(tok, iStart, i);

            const size_t curEnd = tok.SubTokens[i].EndPos() + tok.SubTokens[i].SuffixLen;
            const size_t nextBeg = tok.SubTokens[i + 1].Pos - tok.SubTokens[i + 1].PrefixLen;
            if (curEnd < nextBeg) {
                const TUtf16String delim(tok.Token + curEnd, nextBeg - curEnd);
                PutToken(delim);
            }

            iStart = i + 1;
        }
    }

    if (iStart == 0 && i == tok.SubTokens.size() - 1) {
        PutToken(tok);
    } else {
        AddSubtokensRange(tok, iStart, i);
    }
}

void TTokenizer::AddSubtokensRange(const TWideToken& tok, size_t iFirst, size_t iLast) {
    PutToken(ExtractWideTokenRange(tok, iFirst, iLast));
}

void TTokenizer::FlushTokens() {
    if (!CurSentInfo.Tokens.empty()) {
        Callback.OnSentence(CurSentInfo);
        ++CurSentInfo.SentenceNum;
    }
    NextSentence();
}

// Splits text to blocks using one of the following events:
// 1. Empty line
// 2. Red line (two or more spaces at the beginning)
// 3. Direct speech (dash with leading spaces)
bool TTokenizer::IsBlockStart(const TWtringBuf& line) {
    if (StripString(line).empty())
        return true;
    // Count number of leading spaces. Treat tab as the double space
    size_t leadingSpaces = 0;
    TWtringBuf::const_iterator curChar = line.begin();
    for (; curChar != line.end() && ::IsSpace(curChar, 1); ++curChar) {
        leadingSpaces += wchar16('\t') == *curChar ? 2 : 1;
    }
    return leadingSpaces > 1 || (leadingSpaces != 0 && curChar != line.end() && ::IsDash(*curChar));
}

void TTokenizer::Tokenize(const TUtf16String& text, size_t textPosition) {
    ResetSentence(textPosition);
    TNlpTokenizer tokenizer(*this, false);//false - new tokenizer for multitokens
    tokenizer.Tokenize(text, true);
    FlushTokens();
}

void TTokenizer::ConsumeLine(const TWtringBuf& line) {
    switch (Opts.BlockDetection) {
    case BD_PER_LINE:
        Block.append(line);
        Flush();
        return;
    case BD_DEFAULT:
        if (IsBlockStart(line))
            Flush();
        break;
    case BD_NONE:
        break;
    default:
        Y_FAIL("Unimplemented block detection mode");
        break;
    }
    Block.append(line);
}

void TTokenizer::Flush() {
    if (!Block.empty()) {
        Tokenize(Block, Offset);
        if (!Opts.Has(TF_OFFSET_PER_BLOCK)) {
            Offset += Block.size();
        }
        Block.clear();
    }
}

static bool ReadLine(IInputStream& input, TString& st) {
    char ch;
    if (!input.Read(&ch, 1)) {
        return false;
    }

    st.clear();
    do {
        st += ch;
        if (ch == '\n') {
            break;
        }
    } while (input.Read(&ch, 1));

    return true;
}

static void TokenizeStream(TTokenizer& tokenizer, IInputStream& input, ECharset enc) {
    TString line;

    // Use custom ReadLine, because standard ReadLine silently consumes \r character and
    // we cannot correctly calculate character offset.
    // Also we need '\n' at the end (ReadTo does not include it).
    while (ReadLine(input, line)) {
        tokenizer.ConsumeLine(line, enc);
    }
    tokenizer.Flush();
}

void TokenizeStream(ITokenizerCallback& cb, IInputStream& input, ECharset enc, const TTokenizeOptions& opts, TWorkerPool* pool) {
    if (pool) {
        TPoolCallbackProxy proxy(*pool, cb);
        TTokenizer tokenizer(proxy, opts);
        TokenizeStream(tokenizer, input, enc);
    } else {
        TTokenizer tokenizer(cb, opts);
        TokenizeStream(tokenizer, input, enc);
    }
}

inline void TokenizeText(TTokenizer& tokenizer, const TWtringBuf& text) {
    SplitString(text.data(), text.data() + text.size(), TCharDelimiter<const wchar16>(wchar16('\n')), tokenizer);
    tokenizer.Flush();
}

void TokenizeText(ITokenizerCallback& cb, const TWtringBuf& text, const TTokenizeOptions& opts, TWorkerPool* pool) {
    if (pool) {
        TPoolCallbackProxy proxy(*pool, cb);
        TTokenizer tokenizer(proxy, opts);
        TokenizeText(tokenizer, text);
    } else {
        TTokenizer tokenizer(cb, opts);
        TokenizeText(tokenizer, text);
    }
}

} // NToken
