#pragma once

#include <algorithm>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/generic/utility.h>
#include <util/string/cast.h>
#include <util/charset/wide.h>

#include <library/cpp/token/token_structure.h>
#include <library/cpp/tokenizer/multitokenparser.h>
#include <library/cpp/tokenizer/special_tokens.h>

#include <library/cpp/tokenclassifiers/token_types.h>

#include "req_node.h"
#include "request.h"
#include "reqlenlimits.h"

using TAlphType = wchar16;

class TTokensLengthInfo
{
public:
    TTokensLengthInfo(const TLengthLimits* limits)
        : NumberOfProcessedTokens(0)
        , NumberOfProcessedChars(0)
    {
        if (limits != nullptr) {
            Limits = *limits;
        }
    }

    inline bool LimitExceeded() const {
        return NumberOfProcessedTokens > Limits.MaxNumberOfTokens ||
               NumberOfProcessedChars  > Limits.MaxNumberOfChars;
    }

    inline void Update(const TTokenStructure& subtokens) {
        NumberOfProcessedTokens += subtokens.size();
        for (size_t i = 0; i < subtokens.size(); ++i) {
            NumberOfProcessedChars += subtokens[i].Len + subtokens[i].SuffixLen;
        }
    }

private:
    size_t NumberOfProcessedTokens;
    size_t NumberOfProcessedChars;

    TLengthLimits Limits;
};

//! represetns the prefix of a word or a phrase
struct TLangPrefix
{
    TNodeNecessity Necessity;   //!< represents a necessity operator: '-', '+', '%'
    TFormType FormType;         //!< represents an exactness operator: '!', '!!'
    int LParenPos;              //!< bracket's position in L_PAREN token, -1 in other cases
};

//! represents the postfix of a word or a phrase
struct TLangSuffix
{
    int Idf;        //!< represents a relevance of the document: '::integer'
    TOpInfo OpInfo; //!< describes parent nodes of the request tree: OR, AND, ZONE, ATTR, etc.
                    //!< @todo OpInfo should be removed from this struct and TOpInfo should be added into the byacc union (YYSTYPE) as separate member
    int RParenPos;  //!< stores the bracket's position in R_PAREN token
};

const TLangPrefix DefaultLangPrefix = { nDEFAULT, fGeneral, -1 };
const TLangSuffix DefaultLangSuffix = { -1, { 0, 0, oAnd, 0, cmpEQ, false }, -1 };

//! represents an structure that is filled during request parsing
class TLangEntry
    : public TLangPrefix
    , public TLangSuffix
{
public:
    const wchar16* Text;        //!< whole text of the request
    size_t EntryPos;            //!< the starting position of the current entry (used for attr. names only)
    size_t EntryLeng;           //!< the length of the current entry (used for attr. names only)
    size_t SuffixLeng;          //!< the length of the suffix
    const wchar16* Entry() const {
        if (!Text) return nullptr;
        return Text + EntryPos;
    }
    const wchar16* EntryEnd() const {
        if (!Text) return nullptr;
        return Text + EntryPos + EntryLeng + SuffixLeng;
    }

public:
    explicit TLangEntry(const wchar16* text) : Text(text) {
        Y_ASSERT(Text);
        Reset();
    }
    void Reset() {
        EntryPos = 0;
        EntryLeng = 0;
        SuffixLeng = 0;
        Necessity = nDEFAULT;
        FormType = fGeneral;
        LParenPos = -1;
        Idf = -1;
        OpInfo = ZeroDistanceOpInfo;
        RParenPos = -1;
    }
};

inline bool IsRankingSearchInsideZone(const TLangEntry& e) {
    return (*e.EntryEnd() == '-' && *(e.EntryEnd() + 1) == '>');
}

#include "req_pars.h"

class TReqTokenizer;

TRequestNode* ParseQuotedString(TReqTokenizer& tokenizer, const TLangEntry& extLE);

struct TDeferredToken {
    int Symbol;
    TRequestNode* Node;
    TLangEntry Entry;
    TWideToken Token;

    TDeferredToken(int sym, const wchar16* text)
        : Symbol(sym)
        , Node(nullptr)
        , Entry(text)
    {
    }
    TDeferredToken(int sym, const wchar16* text, const TWideToken& token)
        : Symbol(sym)
        , Node(nullptr)
        , Entry(text)
        , Token(token)
    {
    }
    TDeferredToken(int sym, const TLangEntry& entry, const TWideToken& token)
        : Symbol(sym)
        , Node(nullptr)
        , Entry(entry)
        , Token(token)
    {
    }
};

class TReqTokenizer : protected TMultitokenParser, protected TNonCopyable {
public:
    const tRequest& request;
    TString Error;
    NSearchRequest::TOperatorsFound FoundReqLanguage; //!< whether a request contains characters treated as operators (REQWIZARD-1233)

protected:
    typedef TVector<TDeferredToken> TDeferredTokens;

    TLangEntry LE;
    const TAlphType *p, *pcur, *eof;
    const TAlphType *ts, *te;
    int cs, act;

    const bool RootNode;
    TRequestNode::TGcRef GC; //!< only one instance of GC exists to process one request
    THolder<TRequestNode> Result;

    //! the softness attribute of the request that is specified as "softness:50"
    ui32 Softness;
    // the prefix attribute of the request that is specified as "prefix:12345"
    TVector<ui64> KeyPrefix;
    const TAlphType* SepEnd;
    const TAlphType* RestartAsciiEmoji = nullptr;
    TVector<TRequestNode*> Refines;

    const int IdentSymbol; //!< it is equal to IDENT from req_pars.h
    bool SpacesFound;
    TDeferredTokens DeferredTokens; //!< only the last token can be non-IDENT
    NTokenClassification::TTokenTypes TokenTypes;
    size_t DeferredIndex;
    size_t SubtokenCount;

    TTokensLengthInfo ProccessedTokensLengthInfo;
    bool EnableExtendedSyntax;
    bool EnableAsciiEmoji = false;
    TMap<TString, ui32> SoftnessData;
public:

    //! constructs an instance of request tokenizer
    //! @note this constructor can be used to parse a fragment of a text
    //! @param text         the text to be parsed
    //! @param req          the request object
    //! @param offset       the start position in the text
    TReqTokenizer(const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory = nullptr, size_t offset = 0, const TLengthLimits* limits = nullptr)
        : request(req)
        , LE(text.c_str())
        , p(nullptr)
        , eof(nullptr)
        , RootNode(!nodeFactory)
        , GC(nodeFactory ? nodeFactory : new TRequestNode::TGc())
        , Softness(0)
        , SepEnd(text.c_str())
        , IdentSymbol(IDENT)
        , SpacesFound(false)
        , TokenTypes(0)
        , DeferredIndex(0)
        , SubtokenCount(0)
        , ProccessedTokensLengthInfo(limits)
        , EnableExtendedSyntax(req.GetProcessingFlag() & RPF_ENABLE_EXTENDED_SYNTAX)
    {
        Y_ASSERT(offset < text.length()); // @todo consider throwing an exception instead
        pcur = text.c_str() + offset;
    }

    virtual TReqTokenizer* Clone(const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory = nullptr, size_t offset = 0, const TLengthLimits* limits = nullptr) const = 0;

    virtual ~TReqTokenizer() {
        if (Result && !Result->Empty() && !RootNode) {
            Y_UNUSED(Result.Release());    // will be deleted by GC
        }
    }

    int GetToken(void* val) {
        if (request.UseTrimExtraTokens()) {
            while (true) {
                const int sym = GetTokenImpl(val);
                if (sym != IDENT)
                    return sym;

                // count tokens and characters
                ProccessedTokensLengthInfo.Update(GetSubtokens());
                if (!ProccessedTokensLengthInfo.LimitExceeded())
                    return sym;
                ClearSubtokens();
            }
        }
        return GetTokenImpl(val);
    }

    inline void SetResult(TRequestNode* val) noexcept {
        if (val == nullptr)
            return;

        LoadRefine(&val);
        // It may become NULL after cleaning refine factors
        if (val == nullptr)
            return;

        ApplySoftness(*val);
        val->SetKeyPrefix(KeyPrefix);
        Result.Reset(val);

        if (RootNode)
            GC.Swap(Result->GC); // the root node will contain GC
        else
            Y_UNUSED(GC.Release()); // just release GC
    }

    //! returns result node that can be the root node containing GC or a node not containing GC
    //! @attention if returned node is the root then you must immediately put it to a smart pointer
    inline TRequestNode* GetResult() noexcept {
        TRequestNode* root = Result.Release();
        Y_ASSERT((GC.Get() && !root) || (root && ((root->GC.Get() && root->GC->RefCount() == 1) || !root->GC)));
        return root;
    }

    //! @note the last softness value that was met in the request will be applied
    void SetSoftness(ui32 value) {
        Y_ASSERT(value <= SOFTNESS_MAX);
        Softness = value;
    }

    //! @note the last prefix value that was met in the request will be applied
    void SetKeyPrefix(const TVector<ui64>& value) {
        KeyPrefix = value;
    }

    void AddKeyPrefix(ui64 value) {
        KeyPrefix.push_back(value);
    }

    const TVector<ui64>& GetKeyPrefix() {
        return KeyPrefix;
    }

    //! @note the member function is used by the scanner (see clear_softness action)
    void ResetSoftness() {
        Softness = 0;
    }
    //! @note the member function is used by the scanner (see update_softness action)
    void UpdateSoftness(wchar16 fc) {
        Y_ASSERT(fc >= '0' && fc <= '9');
        Softness = Softness * 10 + (fc - '0');
        if (Softness > SOFTNESS_MAX)
            Softness = SOFTNESS_MAX;
    }

    const TLangEntry& GetEntry() const {
        return LE;
    }

    bool IsNewSyntax() const {
        return false;
    }

    //! @param suffix       a member of the YYSTYPE union: lval.suffix
    void ProcessOperatorWithDistance(TOperType op, TLangSuffix& suffix) {
        if (LE.OpInfo.Lo > LE.OpInfo.Hi)
            DoSwap(LE.OpInfo.Lo, LE.OpInfo.Hi);
        ProcessOperator(op, suffix);
    }

    void ProcessOperator(TOperType op, TLangSuffix& suffix) {
        FoundReqLanguage |= NSearchRequest::TOperatorsFound::Binary;
        FoundReqLanguage |= op;
        LE.OpInfo.Op = op;
        suffix = LE;
        pcur = te;
    }

    virtual void ProcessSeparator() = 0;

    TRequestNode::TFactory& GetNodeFactory() const {
        if (!GC)
            ythrow yexception() << "GC is NULL";
        return *GC;
    }

    void SaveRefine(TRequestNode* right) {
        Refines.push_back(right);
    }

    const TTokensLengthInfo& GetProccessedTokensLengthInfo() const {
        return ProccessedTokensLengthInfo;
    }

    TTokensLengthInfo& GetProccessedTokensLengthInfo() {
        return ProccessedTokensLengthInfo;
    }

    bool IsTruncated() const {
        return request.UseTrimExtraTokens() && ProccessedTokensLengthInfo.LimitExceeded();
    }

    NTokenClassification::TTokenTypes
    GetTokenTypes() const {
        return TokenTypes;
    }

    void DropTokenTypes() {
        TokenTypes = 0;
    }

    const TMap<TString, ui32>& GetNgrSoftnessInfo() const {
        return SoftnessData;
    }

    void AddNgrSoftnessInfo(const TString& zoneName, ui32 softnessData) {
        SoftnessData[zoneName] = Min<ui32>(softnessData, 100);
    }

    TRequestNode* CreatePhrase(TRequestNode* left, TLangSuffix ls, TRequestNode* right, TPhraseType pt) {
        if (!left)
            return right;
        if (!right)
            return left;
        if (IsDocAttrNode(*right, ls.OpInfo.Level)) {
            ls.OpInfo = DefaultRestrDocOpInfo;
            return GetNodeFactory().CreateBinaryOper(left, right, DefaultLangPrefix, ls, PHRASE_USEROP);
        } else if (IsDocAttrNode(*left, ls.OpInfo.Level) && right->Necessity != nMUSTNOT) {
            ls.OpInfo = DefaultRestrDocOpInfo;
            return GetNodeFactory().CreateBinaryOper(right, left, DefaultLangPrefix, ls, PHRASE_USEROP);
        } else {
            if (!left->Parens && left->OpInfo.Op == oRestrDoc && IsDocAttr(*left->Left, left->OpInfo.Level)) {
                ls.OpInfo = DefaultRestrDocOpInfo;
                return GetNodeFactory().CreateBinaryOper(right, left, DefaultLangPrefix, ls, PHRASE_USEROP);
            }
            bool wasParens = false;
            if (left->Parens && left->OpInfo.Op == oRestrDoc && left->Necessity == nDEFAULT && left->FormType == fGeneral) {
                left->Parens = false;
                wasParens = true;
            }
            // oRestrDoc right node in parentheses is not considered
            // for example: [a (b c << url:d) e] - the url attribute must not be moved from parentheses
            TRequestNode* res = GetNodeFactory().CreateBinaryOper(left, right, DefaultLangPrefix, ls, pt);
            if (!left->Parens && left->OpInfo.Op == oRestrDoc) {
                res->Left = left->Left;
                left->Left = res;
                if (wasParens)
                    left->Parens = true;
                FixAndNot(left->Left);
                return left;
            } else if (!right->Parens && right->OpInfo.Op == oRestrDoc) {
                res->Right = right->Left;
                right->Left = res;
                FixAndNot(right->Left);
                return right;
            }
            FixAndNot(res);
            return res;
        }
    }

    using TMultitokenParser::GetMultitoken;
    using TMultitokenParser::ClearSubtokens;

protected:
    //! @note implemented in reqscan.rl
    virtual int Tokenize(void* val) = 0;

    //! @note see terminal symbols in req_pars.y
    void AssignUnion(void* val, int sym, const TDeferredToken& deferredToken) {
        LE = deferredToken.Entry;

        YYSTYPE* p = (YYSTYPE*)val;
        switch (sym) {
        default:
            Y_ASSERT(!"unexpected terminal symbol");
            break;
        case -1:
            // do nothing
            break;
        case ATTR_VALUE:
        case ZONE_L_PAREN:
        case ZONE_NAME:
            p->pnode = deferredToken.Node;
            break;
        case OR:
        case AND1:
        case AND2:
        case ANDNOT1:
        case ANDNOT2:
        case REFINE:
        case RESTR_DOC:
        case WORD_VARIANT_OP:
        case R_PAREN:
        case RESTRBYPOS:
            p->suffix = LE;
            break;
        case IDENT:
        case QUOTED_STRING:
            p->pentry = &LE;
            break;
        case L_PAREN:
            p->prefix = LE;
            break;
        }
    }

    //! @note see terminal symbols in req_pars.y
    void AssignToken(TDeferredToken& deferredToken, void* val, int sym) {
        YYSTYPE* p = (YYSTYPE*)val;
        switch (sym) {
        default:
            Y_ASSERT(!"unexpected terminal symbol");
            break;
        case -1:
            // do nothing
            break;
        case ATTR_VALUE:
        case ZONE_L_PAREN:
        case ZONE_NAME:
            deferredToken.Node = p->pnode;
            break;
        case OR:
        case AND1:
        case AND2:
        case ANDNOT1:
        case ANDNOT2:
        case REFINE:
        case RESTR_DOC:
        case WORD_VARIANT_OP:
        case R_PAREN:
        case RESTRBYPOS:
            static_cast<TLangSuffix&>(deferredToken.Entry) = p->suffix;
            break;
        case IDENT:
        case QUOTED_STRING:
            deferredToken.Entry = *p->pentry;
            break;
        case L_PAREN:
            static_cast<TLangPrefix&>(deferredToken.Entry) = p->prefix;
            break;
        }
    }


    //! @return false - stop deferring
    bool DeferToken(int sym, void* val) {
        if (DeferredIndex == DeferredTokens.size()) {
            DeferredTokens.clear();
            DeferredIndex = 0;
        }

        size_t addedSubtokens = 0;
        if (sym == IdentSymbol) {
            const TWideToken& tok = TMultitokenParser::GetMultitoken();
            Y_ASSERT(tok.SubTokens.size() <= MAX_SUBTOKENS);
            addedSubtokens = tok.SubTokens.size();
            SubtokenCount += addedSubtokens;
            DeferredTokens.push_back(TDeferredToken(IdentSymbol, LE.Text, tok));
            AssignToken(DeferredTokens.back(), val, sym);
            ClearSubtokens();
        } else {
            // all members of this class will have values for the last token
            // actually for sym == -1 storing LE not required
            DeferredTokens.push_back(TDeferredToken(sym, LE.Text));
            AssignToken(DeferredTokens.back(), val, sym);
        }

        const bool stopDeferring = (sym != IdentSymbol) // -1 always stops deferring
            || (SpacesFound && addedSubtokens < SubtokenCount) // spaces between the last token and the previous ones (if any)
            || SubtokenCount > MAX_SUBTOKENS;
        SpacesFound = false;

        if (stopDeferring && DeferredTokens.size() > 1) {
            const size_t OFFSET = 2; // last token in DeferredTokens doesn't belong to current multitoken.

            const TDeferredToken& first = *(DeferredTokens.begin());
            const TDeferredToken& last  = *(DeferredTokens.end() - OFFSET);

            const wchar16* tokenBegin = first.Token.Token;
            size_t multitokenLength =
                (last.Token.Token - first.Token.Token) + last.Token.Leng;
            if (ClassifyTokens(tokenBegin, multitokenLength, TokenTypes)) {
                TWideToken classifiedTitoken(tokenBegin, multitokenLength);
                TDeferredToken newDeferredToken(IdentSymbol,
                                                first.Entry.Text,
                                                classifiedTitoken);

                newDeferredToken.Entry = first.Entry;
                DeferredTokens.front() = newDeferredToken;
                DeferredTokens.erase(DeferredTokens.begin() + 1, DeferredTokens.end() - 1);
            }
        }
        return !stopDeferring;
    }

    bool ReturnDeferredToken() const {
        if (DeferredTokens.size() == 1 && DeferredTokens[0].Symbol == IdentSymbol)
            return false;
        return DeferredIndex < DeferredTokens.size();
    }

    //! @note the last symbol always -1 (see TReqTokenizer::Tokenize())
    int GetDeferredToken(void* val) {
        Y_ASSERT(DeferredIndex < DeferredTokens.size());
        const int sym = DeferredTokens[DeferredIndex].Symbol;
        if (sym == IdentSymbol)
            SetMultitoken(DeferredTokens[DeferredIndex].Token);
        AssignUnion(val, sym, DeferredTokens[DeferredIndex]);
        ++DeferredIndex;
        if (DeferredIndex == DeferredTokens.size() - 1 && DeferredTokens[DeferredIndex].Symbol == IdentSymbol) { // the last deferred token
            DeferredTokens.erase(DeferredTokens.begin(), DeferredTokens.end() - 1);
            DeferredIndex = 0;
            SubtokenCount = DeferredTokens[0].Token.SubTokens.size();
        } else if (DeferredIndex == DeferredTokens.size()) {
            DeferredTokens.clear();
            DeferredIndex = 0;
            SubtokenCount = 0;
        }
        return sym;
    }

    int GetTokenImpl(void* val) {
        if (request.UseTokenClassifier()) {
            if (!ReturnDeferredToken()) {
                while (true) {
                    const int tok = Tokenize(val);
                    if (!DeferToken(tok, val))
                        break;
                }
            }
            return GetDeferredToken(val);
        }
        return Tokenize(val);
    }

    int ProcessIdent(YYSTYPE& lval, const TAlphType* entryEnd) {
        LE.OpInfo.Lo = 1;
        LE.OpInfo.Hi = 1;
        lval.pentry = &LE;
        pcur = entryEnd;
        return IDENT;
    }
    int ProcessMultitoken(YYSTYPE& lval) {
        size_t len = 0;
        if (SetRequestMultitoken(ts, te, len))
            return ProcessIdent(lval, te);
        else
            return ProcessIdent(lval, ts + len);
    }
    int ProcessSurrogatePair(YYSTYPE& lval) {
        const size_t n = te - ts;
        AddIdeograph(n);
        size_t len = 0;
        SetRequestMultitoken(ts, te, len); // AddLastToken() won't add token because CurCharSpan.Len == 0
        Y_ASSERT(GetMultitoken().Token == ts && len == n);
        return ProcessIdent(lval, te);
    }
    bool CanStartAsciiEmoji() const {
        if (!EnableAsciiEmoji)
            return false;
        if (p == RestartAsciiEmoji)
            return true; // continuation of already-checked sequence
        // we require that emojis with one of ()"' should start and end with separator or beginning/end of the request,
        // unless they are balanced inside the emoji:
        // [(password|пароль): (hello|здравствуй), (world|мир).] - we don't want parse ):, ), and ). as emojiis
        // same goes for [с. Беседы, ул. Ленинская, Индустриальный парк "Реал" (900 м от МКАД);]
        // [something="quoted string"] is not a query language operator, but we still don't want to consume =" and leave unbalanced "
        int parenthesisBalance = 0, quoteBalance = 0, aposBalance = 0;
        const wchar16* q;
        for (q = p; *q > ' ' && *q < 128 && !IsAlnum(*q); q++) { // *q == 0 is how the rest of machine determines eof
            if (*q == '(')
                parenthesisBalance++;
            else if (*q == ')')
                parenthesisBalance--;
            else if (*q == '"')
                quoteBalance = !quoteBalance;
            else if (*q == '\'')
                aposBalance = !aposBalance;
        }
        if (parenthesisBalance || quoteBalance || aposBalance) {
            return (p == SepEnd) && (*q == 0 || IsSpace(*q));
        }
        /* yet another compatibility hack:
            [<< (service_flag:"ya-main")] is not a valid query language, it is parsed as [<<] AND [service_flag:"ya-main"],
            the old tokenizer ignores [<<] so the end result is attribute-only request which sort of makes sense.
            If the token is "<<", check whether something like an attribute follows, and if so, ignore this "<<".
            Note that this code is not called for valid queries like [something << (attr:"value")].
        */
        if (q - p == 2 && p[0] == '<' && p[1] == '<') {
            const wchar16* attrBegin = q;
            while (*attrBegin == ' ' || *attrBegin == '(')
                ++attrBegin;
            const wchar16* attrEnd = attrBegin;
            while (*attrEnd >= 'a' && *attrEnd <= 'z' || *attrEnd >= '0' && *attrEnd <= '9' || *attrEnd == '_')
                ++attrEnd;
            if (attrEnd != attrBegin && *attrEnd == ':' && IsAttrName(TUtf16String(attrBegin, attrEnd)))
                return false;
        }
        return true;
    }
    int CreateSpecialToken(YYSTYPE& lval, const wchar16* text, size_t len) {
        TWideToken specialToken;
        specialToken.Token = text;
        specialToken.Leng = len;
        for (size_t i = 0; i < len; i++)
            specialToken.SubTokens.push_back(i, 1);
        SetMultitoken(specialToken);
        return ProcessIdent(lval, text + len);
    }
    int ProcessPunctuation(YYSTYPE& lval) {
        CancelToken();
        LE.Reset();
        for (; ts < te; ++ts) {
            size_t n = GetSpecialTokenLength(ts, Min<size_t>(te - ts, MAX_SUBTOKENS));
            if (n) {
                CreateSpecialToken(lval, ts, n); // also resets pcur = ts + n
                // we don't want to restart if only one character is left:
                // no new emojis will be added anyway, but the last character
                // can be '(' or ')' and could be recognized as a wrong token
                if (pcur == te - 1)
                    ++pcur;
                RestartAsciiEmoji = pcur;
                return IDENT;
            }
        }
        return -1; // nothing to see here, move along
    }
    int ProcessQuotedString(YYSTYPE& lval) {
        CancelToken();
        ClearSubtokens();
        LE.OpInfo = DefaultQuoteOpInfo;
        lval.pentry = &LE;
        pcur = te;
        return QUOTED_STRING;
    }
    enum {
        ATTR,
        ZONE,
        ZONE_OR_ATTR,
        VALUE_QUOTED,
        VALUE_IN_PARENTHESES
    };
    template <int T>
    int ProcessZoneOrAttr(YYSTYPE& lval) {
        FoundReqLanguage |= NSearchRequest::TOperatorsFound::Binary;
        CancelToken();
        ClearSubtokens();
        pcur = te;
        return (
            T == ATTR ? CreateAttrNode(*this, te, &lval, true) : (
            T == ZONE ? CreateZoneNode(*this, te, &lval) : CreateZoneOrAttr(*this, te, &lval)));
    }
    int ProcessZoneWithQuotedText(YYSTYPE& lval) {
        FoundReqLanguage |= NSearchRequest::TOperatorsFound::Binary;
        lval.pnode = CreateZoneNode(*this, LE);
        pcur = LE.EntryEnd() + (IsRankingSearchInsideZone(LE) ? 2 : 1);
        // 2 - length of "->", 1 - length of ":"
        return ZONE_NAME;
    }
    int ProcessZoneWithTextInParentheses(YYSTYPE& lval) {
        FoundReqLanguage |= NSearchRequest::TOperatorsFound::Binary;
        lval.pnode = CreateZoneNode(*this, LE);
        Y_ASSERT(LE.OpInfo.CmpOper == cmpEQ);
        pcur = LE.EntryEnd() + (IsRankingSearchInsideZone(LE) ? 3 : 2); // cannot use te here!
        // 3 - length of "->(", 2 - length of ":("
        return ZONE_L_PAREN;
    }
    size_t RemoveTrailingUnderscores(const wchar16* s, size_t n) {
        const wchar16* p = s + n;
        while (p != s && p[-1] == 0x5F) // name of attribute or zone can contain only ASCII underscore symbol (yc_underscore)
            --p;
        return (p - s);
    }
    int ConvertNameToToken(YYSTYPE& lval) {
        const wchar16* const p = LE.Text + LE.EntryPos;
        const TTokenStructure& subtokens = GetSubtokens();
        SetMultitoken(p - subtokens[0].PrefixLen, RemoveTrailingUnderscores(p, LE.EntryLeng) + subtokens[0].PrefixLen);
        return ProcessIdent(lval, LE.EntryEnd());
    }
    // ranking search inside zone: zone->(text)
    template <int T>
    int ProcessZoneSearch(YYSTYPE& lval) {
        FoundReqLanguage |= NSearchRequest::TOperatorsFound::Binary;
        CancelToken();
        if (request.GetAttrType(GetNameByEntry(LE)) == RA_ZONE) {
            ClearSubtokens();
            if (T == VALUE_QUOTED)
                return ProcessZoneWithQuotedText(lval);
            SepEnd = p + 1; // +1 skip left parenthesis
            return ProcessZoneWithTextInParentheses(lval);
        }
        return ConvertNameToToken(lval);
    }
    bool IsAttrName(const TUtf16String& name) const {
        return TReqAttrList::IsAttr(request.GetAttrType(name));
    }
    bool IsZoneName(const TUtf16String& name) const {
        return TReqAttrList::IsZone(request.GetAttrType(name));
    }
    bool IsNameUnknown(const TUtf16String& name) const {
        return request.GetAttrType(name) == RA_UNKNOWN;
    }
    template <int T>
    int CheckAttrName(YYSTYPE& lval, int& sym) {
        Y_ASSERT(*LE.EntryEnd() == ':');
        //Y_ASSERT(fc == '"' || fc == '\'' || fc == '`' || fc == '(');
        const EReqAttrType type = request.GetAttrType(GetNameByEntry(LE));
        if (TReqAttrList::IsZone(type)) {
            ClearSubtokens();
            if (T == VALUE_QUOTED) {
                sym = ProcessZoneWithQuotedText(lval);
            } else {
                SepEnd = p + 1; // +1 skip left parenthesis
                sym = ProcessZoneWithTextInParentheses(lval);
            }
            return false;
        }
        if (TReqAttrList::IsAttr(type))
            return true;
        sym = ConvertNameToToken(lval);
        return false;
    }

    void OnSpaces() {
        SpacesFound = true;
    }

    void OnMiscChar() {
    }

    bool IsDocAttrLeaf(const TRequestNodeBase& n, ui16 level) {
        return IsAttribute(n)
            && (request.IsDocTarg(n.GetTextName()) || request.IsDocUrlTarg(n.GetTextName()) && level != 0);
    }

    ui8 GetNGrammBase(const TRequestNodeBase& n) {
        return (request.GetNGrammBase(n.GetTextName()));
    }

    bool IsDocAttr(const TRequestNode& n, ui16 level) {
        return IsDocAttrLeaf(n, level) || n.OpInfo.Op == oRestrictByPos && IsDocAttrNode(*n.Left, level);
    }

    bool IsDocAttrNode(const TRequestNode& n, ui16 level) {
        return IsDocAttr(n, level)
            || n.Left && IsDocAttrNode(*n.Left, level) && n.Right && IsDocAttrNode(*n.Right, level);
    }

    void FixAndNot(TRequestNode* res) {
        if (res->Right->OpInfo.Op == oAndNot || res->Right->OpInfo.Op == oAnd) {
            TRequestNode* andnot = res->Right;
            TRequestNode* andnotParent = res;
            while (andnot && andnot->Left && (andnot->Left->OpInfo.Op == oAndNot || andnot->Left->OpInfo.Op == oAnd)) {
                andnotParent = andnot;
                andnot = andnot->Left;
            }
            if (andnot && andnot->OpInfo.Op == oAndNot && andnot->Left && IsDocAttrNode(*andnot->Left, andnot->OpInfo.Level)) {
                TRequestNode* temp = andnot->Left;
                andnot->Left = res->Left;
                res->Left = res->Right;
                res->Right = temp;
                res->OpInfo = DefaultRestrDocOpInfo;
                res->SetPhraseType(PHRASE_USEROP);
                if (andnot->Left && andnot->Left->OpInfo.Op == oAnd && andnot->Left->OpInfo.Level == 2) {
                    andnotParent->Left = andnot->Left;
                    andnot->Left = andnot->Left->Right;
                    andnotParent->Left->Right = andnot;
                    andnotParent = andnotParent->Left;
                    while (andnot->Left && andnot->Left->OpInfo.Op == oAnd && andnot->Left->OpInfo.Level == 2) {
                        andnotParent->Right = andnot->Left;
                        andnot->Left = andnot->Left->Right;
                        andnotParent->Right->Right = andnot;
                        andnotParent = andnotParent->Right;
                    }
                }
            }
        }
    }

    void ApplySoftness(TRequestNode& node) {
        if (Softness) {
            Y_ASSERT(Softness <= SOFTNESS_MAX);
            node.SetSoftness(Softness);
        }
    }

    bool IsRefineFactorNode(const TRequestNode* node) {
        Y_ASSERT(nullptr != node);
        return IsAttribute(*node, REFINE_FACTOR_STR);
    }

    void RemoveRefineFactorNodes(TRequestNode*& node, TRequestNode*& factorAttrNode) {
        bool hasLeft = nullptr != node->Left;
        bool hasRight = nullptr != node->Right;
        if (hasLeft)
            RemoveRefineFactorNodes(node->Left, factorAttrNode);
        if (hasRight)
            RemoveRefineFactorNodes(node->Right, factorAttrNode);

        if (hasLeft && nullptr == node->Left) {
            // For zones the Left keeps reference to zone attribute. So, just keep zone as as
            if (oZone != node->Op())
                node = node->Right;
        } else if (hasRight && nullptr == node->Right) {
            if (oZone != node->Op()) {
                node = node->Left;
            } else if (hasLeft) {
                // Zone attribute becomes zone restriction
                node->Right = node->Left;
                node->Left = nullptr;
            } else {
                // Remove zone, which contains only refine factor
                node = nullptr;
            }
        }

        // Check this after verifying Left and Right
        if (nullptr != node && IsRefineFactorNode(node)) {
            factorAttrNode = node;
            node = nullptr;
        }
    }

    void LoadRefine(TRequestNode** root) {
        TRequestNode* refineFactorNode = nullptr;
        // Remove and ignore refine factors from the left part.
        // The root may become NULL if left part consists of only refine factor attr.
        RemoveRefineFactorNodes(*root, refineFactorNode);

        if (*root) {
            const TLangSuffix refineSuffix = { -1, { 0, 0, oRefine, 2, cmpEQ, false }, -1 };
            for (size_t i = 0; i < Refines.size(); ++i) {
                refineFactorNode = nullptr;
                RemoveRefineFactorNodes(Refines[i], refineFactorNode);
                // Ignore right refine parts which consist of only refine factor attr
                if (nullptr != Refines[i]) {
                    *root = GetNodeFactory().CreateBinaryOper(*root, Refines[i], DefaultLangPrefix, refineSuffix, PHRASE_USEROP);
                    if (nullptr != refineFactorNode) {
                        Y_ASSERT(!!refineFactorNode->GetText());
                        SetRefineFactorValue(**root, refineFactorNode->GetText());
                    }
                }
            }
        }
    }

    static void SetRefineFactorValue(TRequestNode& node, const TUtf16String& text) {
        TWtringBuf name, value;
        if (TWtringBuf(text).TrySplit('=', name, value)) {
            float fVal = 0.f;
            if (!value.empty()) {
                try {
                    // Parse and normalize factor value. It must be in the 0..1 range.
                    fVal = ClampVal(FromString<float>(WideToUTF8(value)), 0.f, 1.f);
                } catch (const TFromStringException&) {
                    // Use zero value for bad numbers
                }
            }
            node.SetRefineFactor(ToWtring(name), fVal);
        } else {
            node.SetRefineFactor(text, 1.f);
        }
    }

public:
    virtual TQuotedStringTokenizer* GetQuotedStringTokenizer(const tRequest& req, TRequestNode::TFactory& nodeFactory, const TLangEntry& extle, TTokensLengthInfo& proccessedTokensLengthInfo) const = 0;
    virtual size_t GetVersion() const = 0;

    static TReqTokenizer* GetByVersion(size_t version, const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory = nullptr, size_t offset = 0, const TLengthLimits* limits = nullptr);
};

class TQuotedStringTokenizer : protected TMultitokenParser, protected TNonCopyable {
    enum {
        DEFAULT_DISTANCE = 1
    };
public:
    TQuotedStringTokenizer(const tRequest& req, TRequestNode::TFactory& nodeFactory, const TLangEntry& extle, TTokensLengthInfo& proccessedTokensLengthInfo)
      : request(req)
      , NodeFactory(nodeFactory)
      , res(nullptr)
      , extLE(extle)
      , LE(extle.Text)
      , TokenTypes(0)
      , SubtokenCount(0)
      , ProccessedTokensLengthInfo(proccessedTokensLengthInfo)
      , p(nullptr), pe(nullptr), eof(nullptr)
      , ts(nullptr), te(nullptr)
      , cs(0), act(0)
      , Distance(DEFAULT_DISTANCE)
      , SepEnd(extle.Entry() + 1) // the first char is (yc_quotation|yc_apostrophe|yc_accent)
    {
    }

    virtual ~TQuotedStringTokenizer() {
    }

    virtual TRequestNode* Parse() = 0;

    NSearchRequest::TOperatorsFound FoundReqLanguage; //!< whether a request contains characters treated as operators (REQWIZARD-1233)
protected:
    void AddMultitoken(const TAlphType* ts, const TAlphType* te, const TAlphType*& p) {
        const TTokenStructure& subtokens = GetSubtokens();
        Y_ASSERT(!subtokens.empty());
        size_t len = 0;
        if (!SetRequestMultitoken(ts, te, len))
            p = (ts + len) - 1;
        ProcessMultitoken();
    }
    void AddIdeograph(const TAlphType* ts, const TAlphType* te) {
        Y_ASSERT(GetSubtokens().empty());
        TMultitokenParser::AddIdeograph(te - ts);
        size_t len = 0;
        SetRequestMultitoken(ts, te, len);
        ProcessMultitoken();
    }
    bool CanStartAsciiEmoji() const {
        return EnableAsciiEmoji;
    }
    void CreateSpecialToken(const TAlphType* token, size_t len) {
        TWideToken specialToken;
        specialToken.Token = token;
        specialToken.Leng = len;
        for (size_t i = 0; i < len; i++)
            specialToken.SubTokens.push_back(i, 1);
        SetMultitoken(specialToken);
        ProcessMultitoken();
    }
    void AddPunctuation(const TAlphType* ts, const TAlphType* te, const TAlphType*& p) {
        CancelToken();
        LE.Reset();
        for (; ts < te; ++ts) {
            size_t n = GetSpecialTokenLength(ts, Min<size_t>(te - ts, MAX_SUBTOKENS));
            if (n) {
                CreateSpecialToken(ts, n);
                Y_ASSERT(p == te - 1);
                p = ts + n - 1; // -1 because Ragel will increment p
                return;
            }
        }
    }
    void ProcessMultitoken() {
        const TWideToken& tok = GetMultitoken();
        const TTokenStructure& subtokens = GetSubtokens();
        if (!subtokens.empty()) {
            if (request.UseTrimExtraTokens()) {
                ProccessedTokensLengthInfo.Update(subtokens);
                if (ProccessedTokensLengthInfo.LimitExceeded()) {
                    ClearSubtokens();
                    return;
                }
            }

            // distance is stored to the suffix but actually it is distance between this multitoken the previous one
            // the distance will be used to create "and" operator
            LE.OpInfo.Lo = Distance;
            LE.OpInfo.Hi = Distance;
            if (LE.Necessity == nMUSTNOT) // @todo actually + (nMUST) should be allowed only
                LE.Necessity = nDEFAULT;
            if (request.UseTokenClassifier()) {
                if (SubtokenCount + tok.SubTokens.size() > MAX_SUBTOKENS)
                    CreateNodes();
                SubtokenCount += tok.SubTokens.size();
                DeferredTokens.push_back(TDeferredToken(0, LE, tok)); // Symbol member not used in TQuotedStringTokenizer
            } else
                CreateWordNode(LE, GetMultitoken());
            ClearSubtokens();
            ResetDistance();
        }
        LE.Reset();
    }
    void ResetDistance() {
        Distance = DEFAULT_DISTANCE;
    }
    void SetDistance(const TAlphType* ts, const TAlphType* te) {
        Y_ASSERT(ts && te && ts < te);
        Distance += std::count(ts, te, '*');
    }
    void OnSpaces() {
        CreateNodes();
    }
    void OnMiscChar() {
        CancelToken();
        LE.Reset();
    }
    void OnFinish() {
        CreateNodes();
    }
    void CreateNodes() {
        if (!request.UseTokenClassifier())
            return;

        if (DeferredTokens.empty())
            return;

        const TDeferredToken& first = DeferredTokens.front();
        const TDeferredToken& last  = DeferredTokens.back();

        const wchar16* tokenStart = first.Token.Token;
        size_t multitokenLength =
            (last.Token.Token - first.Token.Token) + last.Token.Leng;

        if (ClassifyTokens(tokenStart, multitokenLength, TokenTypes)) {

            TWideToken classifiedToken(tokenStart, multitokenLength);
            CreateWordNode(first.Entry, classifiedToken);
        } else {
            for (size_t i = 0; i < DeferredTokens.size(); ++i)
                CreateWordNode(DeferredTokens[i].Entry, DeferredTokens[i].Token);
        }
        DeferredTokens.clear();
        SubtokenCount = 0;
    }
    void CreateWordNode(const TLangEntry& entry, const TWideToken& token) {
        TRequestNode* const n = NodeFactory.CreateWordNode(entry, token, TokenTypes);
        if (res) {
            TLangSuffix sfx = DefaultLangSuffix;
            sfx.OpInfo.Lo = entry.OpInfo.Lo;
            sfx.OpInfo.Hi = entry.OpInfo.Hi;
            res = NodeFactory.CreateBinaryOper(res, n, DefaultLangPrefix, sfx, PHRASE_USEROP);
            Y_ASSERT(res);
        } else
            res = n;
    }
protected:
    const tRequest& request;
    TRequestNode::TFactory& NodeFactory;
    TRequestNode *res;
    const TLangEntry& extLE;
    TLangEntry LE;
    TVector<TDeferredToken> DeferredTokens;
    NTokenClassification::TTokenTypes TokenTypes;
    size_t SubtokenCount;
    TTokensLengthInfo& ProccessedTokensLengthInfo;
protected:
    const TAlphType *p, *pe, *eof;
    const TAlphType *ts, *te;
    int cs, act;
    int Distance;
    const TAlphType* SepEnd;
    bool EnableAsciiEmoji = false;
};

template<size_t VERSION> class TVersionedQuotedStringTokenizer: public TQuotedStringTokenizer {
public:
    using TQuotedStringTokenizer::TQuotedStringTokenizer;
    virtual TRequestNode* Parse() override;
};

template<>
class TVersionedQuotedStringTokenizer<5> : public TVersionedQuotedStringTokenizer<1> {
public:
    TVersionedQuotedStringTokenizer(const tRequest& req, TRequestNode::TFactory& nodeFactory, const TLangEntry& extle, TTokensLengthInfo& proccessedTokensLengthInfo)
        : TVersionedQuotedStringTokenizer<1>(req, nodeFactory, extle, proccessedTokensLengthInfo)
    {
        EnableAsciiEmoji = true;
    }
};

template<size_t _VERSION> class TVersionedReqTokenizer: public TReqTokenizer {
public:
    using TReqTokenizer::TReqTokenizer;

    static constexpr size_t VERSION = _VERSION;
protected:
    int Tokenize(void* val) override;
    void ProcessSeparator() override;

public:
    TQuotedStringTokenizer* GetQuotedStringTokenizer(const tRequest& req, TRequestNode::TFactory& nodeFactory, const TLangEntry& extle, TTokensLengthInfo& proccessedTokensLengthInfo) const override {
        return new TVersionedQuotedStringTokenizer<VERSION>(req, nodeFactory, extle, proccessedTokensLengthInfo);
    }

    size_t GetVersion() const override {
        return VERSION;
    }

    TReqTokenizer* Clone(const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory = nullptr, size_t offset = 0, const TLengthLimits* limits = nullptr) const override {
        return new TVersionedReqTokenizer(text, req, nodeFactory, offset, limits);
    }
};

template<> class TVersionedReqTokenizer<5> : public TVersionedReqTokenizer<1> {
public:
    static constexpr size_t VERSION = 5;
    TVersionedReqTokenizer(const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory = nullptr, size_t offset = 0, const TLengthLimits* limits = nullptr)
        : TVersionedReqTokenizer<1>(text, req, nodeFactory, offset, limits)
    {
        EnableAsciiEmoji = true;
    }

    TQuotedStringTokenizer* GetQuotedStringTokenizer(const tRequest& req, TRequestNode::TFactory& nodeFactory, const TLangEntry& extle, TTokensLengthInfo& proccessedTokensLengthInfo) const override {
        return new TVersionedQuotedStringTokenizer<VERSION>(req, nodeFactory, extle, proccessedTokensLengthInfo);
    }

    size_t GetVersion() const override {
        return VERSION;
    }

    TReqTokenizer* Clone(const TUtf16String& text, const tRequest& req, TRequestNode::TFactory* nodeFactory = nullptr, size_t offset = 0, const TLengthLimits* limits = nullptr) const override {
        return new TVersionedReqTokenizer<5>(text, req, nodeFactory, offset, limits);
    }
};

using TDefaultReqTokenizer = TVersionedReqTokenizer<5>;

typedef  TVector<TCharSpan> TCharSpanVector;

