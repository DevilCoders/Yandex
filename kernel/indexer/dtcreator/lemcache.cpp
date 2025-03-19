#include <kernel/search_types/search_types.h>
#include "lemcache.h"
#include "tokenproc.h"

#include <kernel/indexer/direct_text/fl.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/token/charfilter.h>
#include <kernel/keyinv/invkeypos/keyconv.h>
#include <ysite/yandex/common/prepattr.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

struct TWideTokenInfo {
    const TWideToken& Token;
    EJoinType Join;     // join type of whole multitoken
    bool LeftDelim;     // delimiter at the beginning of multitoken
    wchar16 RightDelim; // delimiter at the end of multitoken

    explicit TWideTokenInfo(const TWideToken& wtok, EJoinType join = JOIN_NONE, bool leftDelim = false, wchar16 rightDelim = 0)
        : Token(wtok)
        , Join(join)
        , LeftDelim(leftDelim)
        , RightDelim(rightDelim)
    {}
};

//! combines join information into single byte
inline ui8 GetJoins(const TWideTokenInfo& tokInfo) {
    ui8 joins = 0;
    if (tokInfo.Join & JOIN_LEFT) {
        joins |= FORM_LEFT_JOIN;
        if (tokInfo.LeftDelim)
            joins |= FORM_LEFT_DELIM;
    }
    if (tokInfo.Join & JOIN_RIGHT) {
        joins |= FORM_RIGHT_JOIN;
        if (tokInfo.RightDelim)
            joins |= FORM_RIGHT_DELIM;
    }
    return joins;
}

struct TTokenKey2 {
    // в зависимости от внешнего langMask, мультитокены могут иметь разную лемматизацию... Жесть.
    const wchar16* Token;
    TLangMask   LangMask;
    ELanguage FirstLang;
    ui16        Len;
    ui8         LemmerOpt; //!< index of lemmer options in TLemmatizationCache::LemmerOptions, 0xFF for non-lemmer tokens
    ui8         Joins;

    TTokenKey2() {}
    TTokenKey2(const TWideTokenInfo& tokInfo, TLangMask langMask, const ELanguage firstLang, ui8 lemmerOpt)
        : LemmerOpt(lemmerOpt)
    {
        Token = tokInfo.Token.Token;
        Len = (ui16)tokInfo.Token.Leng; // prefix included
        LangMask = langMask;
        FirstLang = firstLang;
        Joins = GetJoins(tokInfo);
    }

    struct THash {
        size_t operator()(const TTokenKey2& ke) const {
            return ComputeHash(TWtringBuf{ke.Token, ke.Len}) ^
                IntHash(ke.LemmerOpt) ^ ke.LangMask.GetHash() ^ IntHash((int)ke.FirstLang) ^ IntHash(ke.Joins);
        }
    };

    bool operator ==(const TTokenKey2& y) const {
        return Len == y.Len
            && LangMask == y.LangMask
            && FirstLang == y.FirstLang
            && LemmerOpt == y.LemmerOpt
            && memcmp(Token, y.Token, Len*sizeof(wchar16)) == 0
            && Joins == y.Joins;
    }
};

} // namespace NIndexerCore
} // namespace NIndexerCorePrivate

namespace NIndexerCore {
namespace NIndexerCorePrivate {

class TLemmatizationCache::THashedStringStorage {
private:
    using TTextKey = TStringBuf;
    using THash = THash<TStringBuf>;
    struct TEqualTo {
        size_t operator()(const TTextKey& a, const TTextKey& b) const {
            return a == b;
        }
    };
    typedef THashSet<TTextKey, THash, TEqualTo, TPoolAllocator> TStringHash;
private:
    TMemoryPool* Pool;
    TStringHash StringHash;
public:
    THashedStringStorage(TMemoryPool* pool, size_t hashSize)
        : Pool(pool)
        , StringHash(pool, hashSize)
    {
    }
    TStringBuf Store(const char* text, size_t textLen) {
        std::pair<TStringHash::iterator, bool> ins = StringHash.insert(TTextKey(text, textLen));
        TTextKey& newText = const_cast<TTextKey&>(*ins.first);
        if (ins.second)
            newText = TTextKey(Pool->AppendCString(TStringBuf(text, textLen)).data(), newText.size());
        return TStringBuf(newText.data(), textLen);
    }
    void Clear() {
        StringHash = TStringHash(Pool);
    }
    void Reserve(size_t hashSize) {
        StringHash.reserve(hashSize);
    }
};

class TLemmatizationCache::TTokenHash : public THashMap<TTokenKey2, TLemmatizedTokens, TTokenKey2::THash, TEqualTo<TTokenKey2>, TPoolAllocator> {
private:
    typedef THashMap<TTokenKey2, TLemmatizedTokens, TTokenKey2::THash, TEqualTo<TTokenKey2>, TPoolAllocator> TBase;
public:
    TTokenHash(TMemoryPool* pool, size_t hashSize)
        : TBase(pool, hashSize)
    {
    }
};

class TDefaultTokenProcessor : public ITokenProcessor {
public:
    void Lemmatize(const TWideToken& token, TWLemmaArray& lemmas, const TLangMask& langMask, const ELanguage* langs, const NLemmer::TAnalyzeWordOpt& options) override {
        NLemmer::AnalyzeWord(token, lemmas, langMask, langs, options);
    }
};

static TDefaultTokenProcessor DefTokenProc;

TLemmatizationCache::TLemmatizationCache(const TDTCreatorConfig& cfg, const TCompatiblePureContainer* pureContainer,
    const TIndexLanguageOptions& langOptions, const NLemmer::TAnalyzeWordOpt* lOpt, size_t lOptCount, size_t blockSize)
    : Pool(blockSize)
    , CfgHashSize(cfg.CacheHashSize)
    , StringStorage(new THashedStringStorage(&Pool, CfgHashSize))
    , TokenHash(new TTokenHash(&Pool, CfgHashSize))
    , PureContainer(pureContainer)
    , CurrentPure(PureContainer->GetCompatible(langOptions.GetLanguage()))
    , LemmerOptions(lOpt, lOpt + lOptCount)
    , LanguageOptions(langOptions)
    , TokenProc(&DefTokenProc)
    , LemmaNormalization(true)
{
    Y_ASSERT(lOptCount != 0 && lOptCount <= 0xFF); // index 0xFF is reserved for non-lemmer tokens
}

TLemmatizationCache::~TLemmatizationCache() {
}

void TLemmatizationCache::SetLang(ELanguage lang) {
    if (lang != LanguageOptions.GetLanguage() || !CurrentPure.IsDefined()) {
        LanguageOptions.SetLang(lang);
        CurrentPure = PureContainer->GetCompatible(lang);
    }
}

TIndexLanguageOptions TLemmatizationCache::SetLanguageOptions(const TIndexLanguageOptions& newOptions) {
    TIndexLanguageOptions oldOptions = LanguageOptions;
    if (oldOptions.GetLanguage() != newOptions.GetLanguage() || !CurrentPure.IsDefined()) {
        CurrentPure = PureContainer->GetCompatible(newOptions.GetLanguage());
    }
    LanguageOptions = newOptions;
    return oldOptions;
}

void TLemmatizationCache::SetLemmaNormalization(bool value, const NLemmer::TAnalyzeWordOpt* options, size_t count) {
    Y_ASSERT(LemmaNormalization != value && LemmerOptions.size() == count);
    LemmaNormalization = value;
    LemmerOptions.assign(options, options + count);
}

void TLemmatizationCache::Restart() {
    // The only way to clear both nodes and buckets.
    *TokenHash = TTokenHash(&Pool, 0);
    StringStorage->Clear();
    Pool.Clear();
    TokenHash->reserve(CfgHashSize);
    StringStorage->Reserve(CfgHashSize);
}

void TLemmatizationCache::SetTokenProc(ITokenProcessor* proc) {
    TokenProc = proc;
}

void TLemmatizationCache::FillNonLemmerLematizedToken(const TWideTokenInfo& tokInfo, NLP_TYPE nlpType, TLemmatizedToken& lemToken) {
    lemToken = TLemmatizedToken();
    // @todo replace TUtf16String with wchar16 buffer
    const TTokenStructure& subTokens = tokInfo.Token.SubTokens;
    const size_t prefixLen = subTokens.empty() ? 0 : subTokens[0].PrefixLen;
    TUtf16String forma(tokInfo.Token.Token + prefixLen, Min(tokInfo.Token.Leng - prefixLen, (size_t)MAXWORD_BUF - 1));
    forma.to_lower();

    wchar16 lemma[MAXWORD_BUF];
    const size_t lemmaLen = PrepareNonLemmerToken(nlpType, forma.data(), lemma);
    if (!lemmaLen)
        return; // Does it happen?

    lemToken.TermCount = CurrentPure.CalculateTermCount(lemma, lemmaLen, LANG_UNK);

    char tempbuf[MAXWORD_BUF];
    TFormToKeyConvertor convertor(tempbuf, sizeof(tempbuf));
    {
        size_t written = 0;
        convertor.Convert(lemma, lemmaLen, written);
        lemToken.LemmaText = StringStorage->Store(tempbuf, written).data();
    }
    {
        size_t written = 0;
        convertor.Convert(forma.data(), forma.size(), written);
        lemToken.FormaText = StringStorage->Store(tempbuf, written).data();
        lemToken.Flags = 0;
        lemToken.Joins = 0;
        lemToken.Prefix = 0;
#ifndef ROBOT_OLDTOK
        // but there will be required one more byte for flags...
        if (tokInfo.Join != JOIN_NONE && written < MAXWORD_BUF - 2) {
            lemToken.Flags = FORM_HAS_JOINS;
            lemToken.Joins = GetJoins(tokInfo);
        }
        if (prefixLen) {
            Y_ASSERT(prefixLen == 1 && tokInfo.Token.Token[0] < 0x7f);
            lemToken.Prefix = (ui8)tokInfo.Token.Token[0];
        }
#endif
    }
    lemToken.FormOffset = 0;
    // all other fields are default
}

//! see also VerifyLemmas()
template <typename TCharType>
bool CharactersValid(const TCharType* p, size_t n) {
    const TCharType* e = p + n;
    while (p != e) {
        const ui32 c = *p;
        if (c <= 0x20 || c == 0x7F)
            return false;
        ++p;
    }
    return true;
}

bool TLemmatizationCache::CheckHashTable(const TWideTokenInfo& tokInfo, NLP_TYPE nlpType, TTokenText& token, TLemmatizedTokens*& tokForms, ui8 opt) {
    const TWideToken& tok = tokInfo.Token;
    TTokenKey2 key;
    const ELanguage lang = LanguageOptions.GetLanguage(); // use language of the current pure
    if (tok.SubTokens.size() == 1 && nlpType != NLP_WORD) {
        key = TTokenKey2(tokInfo, TLangMask(), lang, 0xFF);
    } else {
        // token without prefix?
        key = TTokenKey2(tokInfo, LanguageOptions.GetLangMask(), lang, opt);
    }
    std::pair<TTokenHash::iterator, bool> ins = TokenHash->insert(TTokenHash::value_type(key, TLemmatizedTokens()));
    if (ins.second) { // new?
        const_cast<const wchar16*&>(ins.first->first.Token) = Pool.AppendCString(TWtringBuf(key.Token, key.Len)).data(); // TODO: by StringStorage
    }
    token.Text = ins.first->first.Token;
    token.Len = key.Len;
    tokForms = &ins.first->second;
    return ins.second;
}

ui32 TLemmatizationCache::StoreMultiToken(const TWideToken& tok, ui8 opt, TTokenText* tokens, TLemmatizedTokens** forms) {
    Y_ASSERT(CharactersValid(tok.Token, tok.Leng));
    Y_ASSERT(tok.SubTokens.size());
    Y_ASSERT(opt < LemmerOptions.size());
    if (tok.SubTokens.size() == 1) {
        const TCharSpan& subtok = tok.SubTokens[0];
        tokens[0].Index = 0;
        tokens[0].Offset = 0;
        tokens[0].Subtoks = 1;
        if (subtok.Type == TOKEN_WORD) {
            StoreLemmerToken(TWideTokenInfo(tok), opt, tokens[0], forms[0]);
        } else {
            Y_ASSERT(subtok.Type == TOKEN_NUMBER || subtok.Type == TOKEN_MARK || subtok.Type == TOKEN_FLOAT);
            StoreNonLemmerToken(TWideTokenInfo(tok), tokens[0], forms[0]);
        }
        return 1;
    }
    return StoreCompositeMultitoken(tok, opt, tokens, forms);
}

ui32 TLemmatizationCache::StoreMultiToken(const TWideToken& tok, ui8 opt, TLemmatizedTokens** forms) {
    TTempArray<TTokenText> buf(tok.SubTokens.size());
    return StoreMultiToken(tok, opt, buf.Data(), forms);
}

//! @todo in case of "18+" two keys should be added to the index: "18" and "18+" similar to words:
//!       lemmer returns lemmas with '+' and without: "europe+" and "europe"
void TLemmatizationCache::StoreNonLemmerToken(const TWideTokenInfo& tokInfo, TTokenText& token, TLemmatizedTokens*& tokForms) {
    const TWideToken& tok = tokInfo.Token;
    const NLP_TYPE nlpType = DetectNLPType(tok.SubTokens);
    if (!CheckHashTable(tokInfo, nlpType, token, tokForms))
        return;
    // tok must have one subtoken only
    // marks and floats can get here in case of compatible mode
    Y_ASSERT(nlpType == NLP_INTEGER || nlpType == NLP_MARK || nlpType == NLP_FLOAT);
    // TODO: remove accents before tokenization
    TCharFilter<TAccents> f(tok.Leng); // numbers can have accents
    const TWideTokenInfo tokInfo2(f.Filter(tok), tokInfo.Join, tokInfo.LeftDelim, tokInfo.RightDelim);
    tokForms->TokenCount = 1;
    tokForms->Tokens = Pool.AllocateArray<TLemmatizedToken>(tokForms->TokenCount);
    FillNonLemmerLematizedToken(tokInfo2, nlpType, *tokForms->Tokens);
    if (!tokForms->Tokens->LemmaText) {
        tokForms->Tokens = nullptr;
        tokForms->TokenCount = 0;
    }
}

inline ui8 SetLeftFormJoin(ui8 oldJoins, bool delim) {
    return oldJoins | (delim ? (FORM_LEFT_JOIN | FORM_LEFT_DELIM) : FORM_LEFT_JOIN);
}

inline ui8 SetRightFormJoin(ui8 oldJoins, bool delim) {
    return oldJoins | (delim ? (FORM_RIGHT_JOIN | FORM_RIGHT_DELIM) : FORM_RIGHT_JOIN);
}

//! lemmas must not be empty and contain symbols <= 0x20 or equal to 0x7F
//! @note temporary solution until all unicode characters are defined correctly in charclasses_16.rl/charnames.rl/symbols.rl
inline bool VerifyLemmas(const TWLemmaArray& lemmas) {
    for (TWLemmaArray::const_iterator it = lemmas.begin(); it != lemmas.end(); ++it) {
        for (size_t i = 0; i < it->GetTextLength(); ++i) {
            const wchar16 c = it->GetText()[i];
            if (c <= 0x20 || c == 0x7F) {
                return false;
            }
        }
    }
    return !lemmas.empty();
}

inline bool OrderFormByOffset(const TLemmatizedToken& value1, const TLemmatizedToken& value2) {
    return value1.FormOffset < value2.FormOffset;
}

//! @todo copy text if at least one delimiter isn't equal to the required
inline static void ReplaceDelimiters(TWideToken& tok, wchar16* buffer, const wchar16* text, size_t len) {
    const size_t n = Min(size_t(MAXKEY_LEN), len);
    std::char_traits<wchar16>::copy(buffer, text, n);
    buffer[n] = 0;

    const TTokenStructure& subtokens = tok.SubTokens;
    const size_t last = subtokens.size() - 1;
    for (size_t i = 0; i < last; ++i) {
        const ETokenDelim delim = subtokens[i].TokenDelim;
        if (delim != TOKDELIM_NULL) {
            const char c = GetTokenDelimChar(delim);
            buffer[subtokens[i + 1].Pos - 1] = c;
        }
    }
}

void TLemmatizationCache::FillLemmerLematizedToken(const TWideTokenInfo& tokInfo, const TWLemmaArray& origLemmas, TLemmatizedToken* tokens) {
    wchar16 buffer[MAXWORD_BUF];
    char forma[MAXWORD_BUF];
    TFormToKeyConvertor FormConvertor(forma, MAXWORD_BUF);
    char lemma[MAXKEY_BUF];
    TFormToKeyConvertor TextConvertor(lemma, MAXKEY_BUF);

    for (size_t i = 0; i < origLemmas.size(); ++i) {
        const TYandexLemma& ylemma = origLemmas[i];
        TLemmatizedToken& res = tokens[i];

        size_t formLen = 0;
        FormConvertor.Convert(ylemma.GetNormalizedForm(), Min(ylemma.GetNormalizedFormLength(), size_t(MAXWORD_BUF - 1)), formLen);
        res.FormaText = StringStorage->Store(forma, formLen).data();
        res.Lang = (ui8)ylemma.GetLanguage();
        res.Flags = 0;
        res.Joins = 0;
        res.Prefix = 0;
        if (formLen < MAXWORD_BUF - 1) { // if there is no space for flags, CC_TITLECASE is ignored
            if (ylemma.GetCaseFlags() & CC_TITLECASE)
                res.Flags |= FORM_TITLECASE;
        }
#ifndef ROBOT_OLDTOK
        const size_t prefixLen = tokInfo.Token.SubTokens[0].PrefixLen;

        if (formLen < MAXWORD_BUF - 2) { // -2 because 1 byte for joins and 1 byte for flags
            const size_t tokfirst = ylemma.GetTokenPos();   // index of the first token
            const size_t toklast = tokfirst + ylemma.GetTokenSpan() - 1; // index of the last token
            const size_t last = tokInfo.Token.SubTokens.size() - 1;
            if (tokInfo.Join || tokfirst != 0 || toklast != last)
                res.Flags |= FORM_HAS_JOINS;
            if (tokfirst == 0) {
                if (tokInfo.Join & JOIN_LEFT)
                    res.Joins = SetLeftFormJoin(res.Joins, tokInfo.LeftDelim);
            } else {
                const bool delim = GetLeftTokenDelim(tokInfo.Token, tokfirst);
                res.Joins = SetLeftFormJoin(res.Joins, delim);
            }
            if (toklast == last) { // in case of last == 0 it also goes here
                if (tokInfo.Join & JOIN_RIGHT)
                    res.Joins = SetRightFormJoin(res.Joins, tokInfo.RightDelim);
            } else {
                const wchar16 delim = GetRightTokenDelim(tokInfo.Token, toklast);
                res.Joins = SetRightFormJoin(res.Joins, delim);
            }
        }
        if (prefixLen && ylemma.GetTokenPos() == 0) { // store prefix for the first subtoken only
            Y_ASSERT(prefixLen == 1 && tokInfo.Token.Token[0] < 0x7f);
            res.Prefix = (ui8)tokInfo.Token.Token[0];
        }
#else
        Y_UNUSED(tokInfo);
#endif
        Y_ASSERT(ylemma.GetTextLength() < MAXWORD_BUF);
        res.TermCount = CurrentPure.CalculateTermCount(ylemma.GetText(), ylemma.GetTextLength(), ylemma.GetLanguage());
        size_t lemmLen = 0;
        if (LemmaNormalization) {
            const size_t n = NormalizeUnicode(ylemma.GetText(), ylemma.GetTextLength(), buffer, MAXWORD_BUF);
            TextConvertor.Convert(buffer, n, lemmLen);
        } else {
            TextConvertor.Convert(ylemma.GetText(), ylemma.GetTextLength(), lemmLen);
        }
        res.LemmaText = StringStorage->Store(lemma, lemmLen).data();

        res.Weight = ylemma.GetWeight();
        res.StemGram  = ylemma.GetStemGram();
        res.GramCount = (ui32)ylemma.FlexGramNum();
        res.FlexGrams = nullptr;
        if (res.GramCount) {
            char** pGrams = Pool.AllocateArray<char*>(res.GramCount);
            memcpy(&pGrams[0], &ylemma.GetFlexGram()[0], res.GramCount * sizeof(char*));
            res.FlexGrams = pGrams;
        }
        res.IsBastard = (ui32)ylemma.IsBastard();
        res.FormOffset = (ui8)ylemma.GetTokenPos();
    }

    Sort(tokens, tokens + origLemmas.size(), OrderFormByOffset);
}

void TLemmatizationCache::LemmatizeToken(const TWideToken& tok, const TWideTokenInfo& tokInfo, const ui8 opt, TLemmatizedTokens*& tokForms) {
    TWideTokenInfo tokInfo2(tok, tokInfo.Join, tokInfo.LeftDelim, tokInfo.RightDelim);
    TWLemmaArray origLemmas;

    TokenProc->Lemmatize(tokInfo2.Token, origLemmas, LanguageOptions.GetLangMask(), LanguageOptions.GetLangPriorityList(), LemmerOptions[opt]);

    Y_ASSERT(origLemmas.size());
    if (VerifyLemmas(origLemmas)) {
        tokForms->TokenCount = (ui32)origLemmas.size();
        tokForms->Tokens     = Pool.AllocateArray<TLemmatizedToken>(tokForms->TokenCount);

        FillLemmerLematizedToken(tokInfo2, origLemmas, tokForms->Tokens);
    } else {
        tokForms->TokenCount = 0;
    }
}

void TLemmatizationCache::StoreLemmerToken(const TWideTokenInfo& tokInfo, ui8 opt, TTokenText& token, TLemmatizedTokens*& tokForms) {
    const TWideToken& tok  = tokInfo.Token;
    const NLP_TYPE nlpType = DetectNLPType(tok.SubTokens);

    if (!CheckHashTable(tokInfo, nlpType, token, tokForms, opt)) {
        return;
    }

    if (tok.SubTokens.size() == 1 && !TCharFilter<TAccents>::HasChars(tok)) {
        LemmatizeToken(tok, tokInfo, opt, tokForms);
    } else {
        wchar16 buffer[MAXKEY_BUF];
        TWideToken preptok(buffer, tok.Leng, tok.SubTokens);
        ReplaceDelimiters(preptok, buffer, tok.Token, tok.Leng); // not needed in case of SubTokens.size() == 1
        // TODO: remove accents before tokenization
        TCharFilter<TAccents> f(preptok.Leng); // 'tok' must not consist of accent characters only, see TNlpParser::MakeEntry()
        TWideTokenInfo tokInfo2(f.Filter(preptok), tokInfo.Join, tokInfo.LeftDelim, tokInfo.RightDelim);
        TWLemmaArray origLemmas;

        LemmatizeToken(f.Filter(preptok), tokInfo, opt, tokForms);
    }
}

inline static bool IsTokenBreak(const TCharSpan& s) {
    return (s.TokenDelim != TOKDELIM_APOSTROPHE && s.TokenDelim != TOKDELIM_MINUS) || s.Type == TOKEN_NUMBER;
}

inline static bool IsTokenBreak(const TCharSpan& s, const TCharSpan& next) {
    return IsTokenBreak(s) || s.Type != next.Type;
}

ui32 TLemmatizationCache::StoreCompositeMultitoken(const TWideToken& tok, ui8 opt, TTokenText* tokens, TLemmatizedTokens** forms) {
    TWideToken token;
    size_t first = 0;
    const size_t last = tok.SubTokens.size() - 1;
    ui32 index = 0;
    for (size_t i = 0; i <= last; ++i) {
        const TCharSpan& s = tok.SubTokens[i];
        if (i == last || IsTokenBreak(s, tok.SubTokens[i + 1])) {
            const EJoinType join = (i == 0 ? (i == last ? JOIN_NONE : JOIN_RIGHT) :
                (i == last ? (first == 0 ? JOIN_NONE : JOIN_LEFT) : (first == 0 ? JOIN_RIGHT : JOIN_BOTH)));
            const bool leftDelim = GetLeftTokenDelim(tok, first);
            const wchar16 rightDelim = GetRightTokenDelim(tok, i);
            tokens[index].Index = first;
            TWideTokenInfo tokInfo(token, join, leftDelim, rightDelim);
            if (s.Type == TOKEN_NUMBER) {
                Y_ASSERT(i == first && token.SubTokens.empty());
                const size_t startPos = s.Pos - s.PrefixLen;
                tokens[index].Offset = startPos;
                token.Token = tok.Token + startPos; // what about suffix: 18+ ?
                token.Leng = s.Len + s.PrefixLen + s.SuffixLen;
                token.SubTokens.push_back(TCharSpan(s.PrefixLen, s.Len, s.Type, s.TokenDelim, s.Hyphen, s.SuffixLen, s.PrefixLen));
                tokens[index].Subtoks = 1;
                StoreNonLemmerToken(tokInfo, tokens[index], forms[index]);
            } else {
                Y_ASSERT(s.Type == TOKEN_WORD);
                const TCharSpan& firsttok = tok.SubTokens[first];
                const size_t startPos = firsttok.Pos - firsttok.PrefixLen;
                tokens[index].Offset = startPos;
                token.Token = tok.Token + startPos;
                token.Leng = s.EndPos() - startPos + s.SuffixLen;
                // actually they could be copied by memcpy()...
                for (size_t j = first; j <= i; ++j) {
                    const TCharSpan& srctok = tok.SubTokens[j];
                    Y_ASSERT(srctok.SuffixLen == 0 || j == i); // no suffix OR the last subtoken
                    token.SubTokens.push_back(TCharSpan(srctok.Pos - startPos, srctok.Len, srctok.Type,
                        srctok.TokenDelim, srctok.Hyphen, srctok.SuffixLen, srctok.PrefixLen));
                }
                tokens[index].Subtoks = token.SubTokens.size();
                StoreLemmerToken(tokInfo, opt, tokens[index], forms[index]);
            }
            if (forms[index]->TokenCount) // can be 0
                ++index;
            token.SubTokens.clear();
            first = i + 1;
        }
    }
    return index;
}

}
}
