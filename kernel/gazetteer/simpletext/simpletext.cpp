#include "simpletext.h"

#include <library/cpp/token/token_util.h>
#include <util/generic/yexception.h>
#include <util/string/strip.h>

namespace NGzt
{

TSimpleText::TSimpleText(TLangMask lang, EMultiTokenSplit split, bool usePunct)
    : TokenStartPos(0)
    , Tokenizer(*this, false)
    , Languages(lang)
    , MultiTokenSplit(split)
    , UsePunct(usePunct)
{
}

TSimpleText::TSimpleText(const TUtf16String& text, TLangMask lang, EMultiTokenSplit split, bool usePunct)
    : TokenStartPos(0)
    , Tokenizer(*this, false)
    , Languages(lang)
    , MultiTokenSplit(split)
    , UsePunct(usePunct)
{
    Reset(text);
}

void TSimpleText::Reset(const TUtf16String& text) {
    OriginalText = text;
    TokenStartPos = 0;

    Words.clear();
    Tokenizer.Tokenize(OriginalText, true, Languages);
    Cache.Reset();
}

void TSimpleText::InitCache(size_t cacheSizeLimit) {
    Cache.Reset(cacheSizeLimit);
}

static inline bool IsSeparateToken(const TWtringBuf& text) {
    return !::StripString(text).empty()
        && ::IsSpace(text[0]) && ::IsSpace(text.back());
}

// NLP tokenizer parses sequential delimiters and spaces as the single wide token.
// Split such token to a separate tokens (one token per delimiter).
void TSimpleText::PutDelims(const TWtringBuf& punct, bool isBreak, size_t offset) {
    if (UsePunct) {
        for (size_t s = 0; s < punct.length(); ++s) {
            if (!IsSpace(punct[s])) {
                AddWord(TUtf16String(punct.data(), s, 1), offset + s);
            }
        }
    } else if (isBreak || IsSeparateToken(punct)) {
        AddWord(TUtf16String(punct), offset);
    }
}

bool TSimpleText::CheckSplit(const TWideToken& word, size_t pos) const {
    switch (MultiTokenSplit) {
    case Wizard:
        return CheckWideTokenReqSplit(word.SubTokens, pos);
    case Index:
        return CheckWideTokenSplit(word, pos);
    default:
        ythrow yexception() << "Unsupported MultiTokenSplit mode";
    }
    return false;
}

void TSimpleText::PutToken(const TWideToken& tok) {
    Y_ASSERT(!tok.SubTokens.empty() && (tok.SubTokens.size() != 1 || tok.SubTokens[0].Len != 0));

    // Single token.
    if (MultiTokenSplit == None || tok.SubTokens.size() == 1) {
        AddWord(tok, 0);
        return;
    }

    size_t iStart = 0, i = 0;
    for (; i + 1 < tok.SubTokens.size(); ++i) {
        if (CheckSplit(tok, i)) {
            AddWord(ExtractWideTokenRange(tok, iStart, i), GetSubTokenOffset(tok, iStart));

            const size_t curEnd = tok.SubTokens[i].EndPos() + tok.SubTokens[i].SuffixLen;
            const size_t nextBeg = tok.SubTokens[i + 1].Pos - tok.SubTokens[i + 1].PrefixLen;
            if (curEnd < nextBeg) {
                PutDelims(TWtringBuf(tok.Token + curEnd, nextBeg - curEnd), false, curEnd);
            }

            iStart = i + 1;
        }
    }

    if (iStart == 0 && i + 1 == tok.SubTokens.size()) {
        AddWord(tok, 0);
    } else {
        AddWord(ExtractWideTokenRange(tok, iStart, i), GetSubTokenOffset(tok, iStart));
    }
}

void TSimpleText::AddWord(const TWideToken& word, size_t offset) {
    if (MultiTokenSplit != None && UsePunct
        && (word.SubTokens[0].PrefixLen || word.SubTokens.back().SuffixLen)) {
        TWideToken newWord = word;
        TUtf16String prefix;
        TUtf16String suffix;
        if (word.SubTokens[0].PrefixLen) {
            prefix = RemoveWideTokenPrefix(newWord);
            PutDelims(prefix, false, offset);
        }
        if (word.SubTokens.back().SuffixLen) {
            suffix = RemoveWideTokenSuffix(newWord);
        }
        DoAddWord(newWord, offset + prefix.length());
        if (!suffix.empty()) {
            PutDelims(suffix, false, offset + prefix.length() + newWord.Leng);
        }
    } else {
        DoAddWord(word, offset);
    }
}

// adds with caching
void TSimpleText::DoAddWord(const TWideToken& tok, size_t offset) {
    if (!Cache.IsInitialized()) {
        Words.push_back(new TSimpleWord(tok, Languages));
    } else {
        size_t* index = nullptr;
        if (!Cache.LookupAndReserve(TWtringBuf(tok.Token, tok.Leng), index)) {
            *index = Words.size();
            Words.push_back(new TSimpleWord(tok, Languages));
        } else {
            TSimpleSharedPtr<TSimpleWord> cachedWord = Words[*index];
            Words.push_back(new TSimpleWord(*cachedWord));
        }
    }
    Words.back()->SetPosition(TokenStartPos + offset);
}

void TSimpleText::AddWord(const TUtf16String& word, size_t offset) {
    Words.push_back(new TSimpleWord(word));
    Words.back()->SetPosition(TokenStartPos + offset);
}

void TSimpleText::OnToken(const TWideToken& tok, size_t origlan, NLP_TYPE type) {
    switch (type) {
    case NLP_WORD:
    case NLP_FLOAT:
    case NLP_INTEGER:
    case NLP_MARK:
        if (tok.SubTokens.empty() || (tok.SubTokens.size() == 1 && tok.SubTokens[0].Len == 0)) {
            PutToken(TWideToken(tok.Token, tok.Leng));
        } else {
            PutToken(tok);
        }
        break;
    case NLP_END:
    case NLP_SENTBREAK:
    case NLP_PARABREAK:
    case NLP_MISCTEXT:
        PutDelims(TWtringBuf(tok.Token, tok.Leng), GetSpaceType(type) != ST_NOBRK);
        break;
    default:
        Y_FAIL("Unknown NLP_TYPE");
    }

    TokenStartPos += origlan;
}



TSimpleWord::TSimpleWord(const TUtf16String& text) {
    Set(text);
}

TSimpleWord::TSimpleWord(const TWideToken& text, const TLangMask& lang) {
    Set(text, lang);
}

void TSimpleWord::Set(const TWideToken& text, const TLangMask& lang) {
    OriginalText.AssignNoAlias(text.Token, text.Leng);

    if (text.SubTokens.size() > 1) {
        //try to lemmatize as one word
        // AnalyzeWord() clears Lemmas before processing, so don't clear it here
        NLemmer::AnalyzeWord(text.Token, text.Leng, Lemmas, lang);
        if (Lemmas.size() > 0 && !Lemmas[0].IsBastard()) {
            NormalizedText = TWtringBuf(Lemmas[0].GetNormalizedForm(), Lemmas[0].GetNormalizedFormLength());
        } else {
            Lemmas.clear();
            LemmatizeMultiToken(text, lang);
            SimpleNormalize();
        }
    } else {
        // TODO: accept bastards?
        // AnalyzeWord() clears Lemmas before processing, so don't clear it here
        NLemmer::AnalyzeWord(text, Lemmas, lang);

        // use normalization from TYandexLemma, if any
        if (Lemmas.size() > 0) {
            NormalizedText = TWtringBuf(Lemmas[0].GetNormalizedForm(), Lemmas[0].GetNormalizedFormLength());
        } else {
            SimpleNormalize();
        }
    }
}

void TSimpleWord::Set(const TUtf16String& text) {
    OriginalText = text;

    SimpleNormalize();
}


//lemmatize only last part and each lemma to original prefix
bool TSimpleWord::LemmatizeMultiToken(const TWideToken& text, TLangMask lang)
{
    TWLemmaArray lastTokenLemmas;
    const TCharSpan& lastToken = text.SubTokens.back();
    NLemmer::AnalyzeWord(text.Token + lastToken.Pos, lastToken.Len, lastTokenLemmas, lang);
    if(lastTokenLemmas.ysize() == 0)
        return false;

    for(int i = 0 ; i < lastTokenLemmas.ysize(); i++)
    {
        TYandexLemma newLemma = lastTokenLemmas[i];
        NLemmerAux::TYandexLemmaSetter lemmaSetter(newLemma);
        TUtf16String newLemmaText(text.Token, lastToken.Pos);
        newLemmaText += TUtf16String(newLemma.GetText(), newLemma.GetTextLength());
        newLemmaText.to_lower();
        lemmaSetter.SetLemma(newLemmaText.data(), newLemmaText.size(), 0, newLemma.GetPrefLen(), newLemma.GetFlexLen(), newLemma.GetStemGram());
        Lemmas.push_back(newLemma);
    }
    return true;
}

void TSimpleWord::SimpleNormalize() {
    // do minimal normalization (lowercasing) of OriginalText if required
    for (TWtringBuf::const_iterator it = OriginalText.begin(); it != OriginalText.end(); ++it)
        if (::IsUpper(*it)) {
            NormalizedTextHolder.AssignNoAlias(OriginalText.data(), OriginalText.size());
            NormalizedTextHolder.to_lower();
            NormalizedText = NormalizedTextHolder;
            return;
        }
    // otherwise just link to OriginalText
    NormalizedText = OriginalText;
}

} // namespace NGzt
