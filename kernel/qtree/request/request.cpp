#include <util/string/util.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>

#include <util/system/mutex.h>
#include <util/system/spinlock.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/tokenclassifiers/token_classifiers_singleton.h>
#include <kernel/reqerror/reqerror.h>

#include "request.h"
#include "req_node.h"
#include "reqscan.h"
#include "analyzer.h"

#define MAX_RECURSION_LEVEL 300

const TReqAttrList tRequest::DefaultAttrList;

static void CollapseIntervalSearch(TRequestNode& node, size_t level) {
    if (level > MAX_RECURSION_LEVEL) {
        return;
    }

    TRequestNode* left = node.Left;
    TRequestNode* right = node.Right;

    Y_ASSERT(node.OpInfo.Op != oRestrDoc || (node.OpInfo.Op == oRestrDoc && left && right));
    // attr with interval example: link>="one" << link<="two"
    if (node.OpInfo.Op == oRestrDoc
        && IsAttribute(*left) && IsAttribute(*right)
        && left->GetTextName() == right->GetTextName()
        && left->OpInfo.CmpOper == cmpGE
        && right->OpInfo.CmpOper == cmpLE)
    {
        node.SetAttrValues(left->GetTextName(), left->GetText(), right->GetText());
        node.Left = nullptr;
        node.Right = nullptr;
    } else {
        if (left) {
            CollapseIntervalSearch(*left, level + 1);
        }

        if (right) {
            CollapseIntervalSearch(*right, level + 1);
        }
    }
}

static void CollapseIntervalSearch(TRequestNode& node) {
    CollapseIntervalSearch(node, 0);
}

static void RemoveDocAnds(TRequestNode* node) {
    if (node) {
        if (node->GetPhraseType() == PHRASE_USEROP &&
            IsAndOp(*node) &&
            node->OpInfo.Level == 2)
        {
            node->SetPhraseType(PHRASE_NONE);
        }
        RemoveDocAnds(node->Left);
        RemoveDocAnds(node->Right);
    }
}

// Writes punctuations into nodes
static void StorePunctuation(TRequestNode* node, const TUtf16String& request, TRequestNode*& prevnode) {
    if (node->Left)
        StorePunctuation(node->Left, request, prevnode);

    if (IsWordOrMultitoken(*node)) {
        size_t pos = 0;
        if (prevnode)
            pos = prevnode->Span.EndPos();

        node->RemovePrefix();
        TUtf16String septext = request.substr(pos, node->Span.Pos - pos);

        node->SetTextBefore(septext);
        if (prevnode)
            prevnode->SetTextAfter(septext);

        prevnode = node;
    }

    if (node->Right)
        StorePunctuation(node->Right, request, prevnode);
}


// Writes punctuations into nodes
static void StorePunctuation(TRequestNode* node, const TUtf16String& request) {
    TRequestNode* prevnode = nullptr;
    StorePunctuation(node, request, prevnode);
    if (prevnode) {
        // Add the last token
        size_t pos = prevnode->Span.EndPos();
        TUtf16String trailer = request.substr(pos);
        prevnode->SetTextAfter(trailer);
    }
}

class TCreateTreeTokenizer: public ITokenHandler {
private:
    TRequestNode::TGcRef GC;
    //smart ptr not needed - all nodes will be destroyed by GC
    TRequestNode* Root = nullptr;

    const TNlpTokenizer* Tokenizer;

    bool CreateNodesFromSimpleWords = false;

    TRequestNode* NodeFromToken(const TWideToken& token) {
        TLangEntry le(Tokenizer->GetTextStart());
        return GC->CreateWordNode(le, token);
    }
public:
    TCreateTreeTokenizer(TRequestNode::TGcRef& gc, bool createNodesFromSimpleWords=false)
        : GC(gc)
        , CreateNodesFromSimpleWords(createNodesFromSimpleWords)
    {
    }

    void OnToken(const TWideToken& token, size_t /*origleng*/, NLP_TYPE type) override {
        if (token.SubTokens.empty() && (!CreateNodesFromSimpleWords || type != NLP_WORD)) {
            return;
        }

        TRequestNode* node;
        if (token.SubTokens.empty()) {
            node = NodeFromToken(TWideToken(token.Token, token.Leng));
        } else {
            node = NodeFromToken(token);
        }

        if (!Root) {
            Root = node;
        } else {
            TLangPrefix langPrefix = DefaultLangPrefix;
            TLangSuffix langSuffix = DefaultLangSuffix;
            langSuffix.OpInfo.Level = 2;
            TRequestNode* opNode = GC->CreateBinaryOper(Root, node, langPrefix, langSuffix, TPhraseType::PHRASE_PHRASE);
            Root = opNode;
        }
    }

    TRequestNode* GetRoot() const {
        return Root;
    }

    void SetTokenizer(const TNlpTokenizer* tokenizer) {
        Tokenizer = tokenizer;
    }
};

tRequest::TNodePtr tRequest::Parse(const TUtf16String& reqText, const TLengthLimits* limits, bool* truncated, size_t tokenizerVersion, NSearchRequest::TOperatorsFound* foundReqLang, bool postProcess) const {
    if (reqText.empty())
        throw TError(yxREQ_EMPTY);

    if (tokenizerVersion >= 2 && tokenizerVersion != 5) {
        TRequestNode::TGcRef GC = new TRequestNode::TGc();
        TCreateTreeTokenizer handler(GC, tokenizerVersion >= 3);
        TNlpTokenizer tokenizer(handler, false);
        handler.SetTokenizer(&tokenizer);
        TTokenizerOptions tokenizerOptions;
        tokenizerOptions.Version = tokenizerVersion;
        tokenizerOptions.UrlDecode = false;
        if (tokenizerVersion == 4) {
            tokenizerOptions.Version = 3;
            tokenizerOptions.KeepAffixes = true;
        }
        tokenizer.Tokenize(reqText.data(), reqText.size(), tokenizerOptions);
        TNodePtr res = handler.GetRoot();
        if (res) {
            GC.Swap(res->GC);
        } else {
            //todo: ythrow
            throw TError(yxREQ_EMPTY); // f.e (???)//6
        }

        if (!postProcess) {
            return res;
        }

        StorePunctuation(res.Get(), reqText);
        CollapseIntervalSearch(*res);
        RunAnalyzer(res);

        return res;
    }

    THolder<TReqTokenizer> toker (TReqTokenizer::GetByVersion(tokenizerVersion, reqText, *this, nullptr, 0, limits));
    if (ParseYandexRequest(toker.Get())) {
        throw TSyntaxError() << toker->Error;
    }

    if (!toker->Error.empty() &&
        (ProcessingFlag & RPF_CLEAN_REQUESTS_ONLY)) {
        throw TSyntaxError() << toker->Error;  // don't recover from syntax errors
    }

    TNodePtr res = toker->GetResult();
    if (!res.Get())
        throw TError(yxREQ_EMPTY); // f.e (???)//6

    if (!postProcess) {
        return res;
    }

    if (ProcessingFlag & RPF_REMOVE_DOCANDS) {
        RemoveDocAnds(res.Get());
    }

    StorePunctuation(res.Get(), reqText);
    CollapseIntervalSearch(*res);
    res->UpdateKeyPrefix(toker->GetKeyPrefix());
    res->UpdateNGrammInfo(AttrList, toker->GetNgrSoftnessInfo());
    RunAnalyzer(res);

    if (truncated != nullptr) {
        *truncated = toker->IsTruncated();
    }

    if (foundReqLang)
        *foundReqLang = toker->FoundReqLanguage;

    return res;
}

tRequest::TNodePtr tRequest::Analyze(const wchar16* reqText, const TLengthLimits* limits, bool* truncated, size_t tokenizerVersion, NSearchRequest::TOperatorsFound* foundReqLang) const {
    TNodePtr res = Parse(reqText, limits, truncated, tokenizerVersion, foundReqLang);
    res->TreatPhrase(PhraseOperInfo, DefaultType);
    return res;
}

static const TOpInfo& GetPhraseOperInfo(ui32 flags) {
    return flags & RPF_DOCUMENT_PHRASE_LEVEL ? DocumentPhraseOpInfo
        : flags & RPF_SENTENCE_PHRASE_LEVEL ? SentencePhraseOpInfo
        : flags & RPF_EXACT_PHRASE_LEVEL ? DefaultQuoteOpInfo
        : DefaultPhraseOpInfo;
}

static TPhraseType GetDefaultType(ui32 flags) {
    return flags & RPF_DOCUMENT_PHRASE_LEVEL ? PHRASE_USEROP
        : flags & RPF_SENTENCE_PHRASE_LEVEL ? PHRASE_USEROP
        : flags & RPF_EXACT_PHRASE_LEVEL ? PHRASE_USEROP
        : PHRASE_PHRASE;
}

tRequest::tRequest(ui32 flags, const TReqAttrList* attrList)
    : ProcessingFlag(flags)
    , PhraseOperInfo(::GetPhraseOperInfo(flags))
    , DefaultType(::GetDefaultType(flags))
    , AttrList(attrList ? *attrList : DefaultAttrList)
{
}

class TAttrListCache : private TNonCopyable {
    //! @note TSharedPtr is used inside of this class only so there is used TSimpleCounter
    typedef THashMap<TString, TSimpleSharedPtr<TReqAttrList> > TCache;
    //! @note spin lock is used because at first there are no calls from different threads
    typedef TSpinLock TLock;
    TCache Cache;
    TLock Lock;
public:
    TReqAttrList* Get(const char* str) {
        TGuard<TLock> guard(Lock);
        TCache::iterator it = Cache.find(str);
        if (it != Cache.end())
            return it->second.Get();
        TSimpleSharedPtr<TReqAttrList> p(new TReqAttrList(str));
        std::pair<TCache::iterator, bool> ret = Cache.emplace(str, p);
        return ret.first->second.Get();
    }
};

tRequest::TNodePtr tRequest::Analyze(const wchar16* reqText, const TReqAttrList* attrList, const TLengthLimits* limits, bool* truncated, size_t tokenizerVersion, NSearchRequest::TOperatorsFound* foundReqLang) const {
    return tRequest(GetProcessingFlag(), attrList ? attrList : &DefaultAttrList).Analyze(reqText, limits, truncated, tokenizerVersion, foundReqLang);
}

tRequest::TNodePtr tRequest::Analyze(const wchar16* reqText, const char* attrList, const TLengthLimits* limits, bool* truncated, size_t tokenizerVersion, NSearchRequest::TOperatorsFound* foundReqLang) const {
    if (!attrList)
        return tRequest(GetProcessingFlag()).Analyze(reqText, limits, truncated, tokenizerVersion, foundReqLang);

    return tRequest(GetProcessingFlag(), Singleton<TAttrListCache>()->Get(attrList)).Analyze(reqText, limits, truncated, tokenizerVersion, foundReqLang);
}

template <typename TChr>
TBasicString<TChr> FromW(const TUtf16String& s) {
    return s;
}

template <>
TBasicString<char> FromW<char>(const TUtf16String& s) {
    return WideToChar(s, CODES_YANDEX);
}

// Обход дерева разбора слева направо
template <typename TChr>
void PickupWords_(const TRequestNode* node, TVector<TBasicString<TChr>>& words, TVector<TBasicString<TChr>>& spaces)
{
    if (node->Left)
        PickupWords(node->Left, words, spaces);

    if (IsWordOrMultitoken(*node)) {
        if (spaces.empty()) // very first node
            spaces.emplace_back();
        words.push_back(FromW<TChr>(node->GetText()));
        spaces.push_back(FromW<TChr>(node->GetPunctAfter()));
    }

    if (node->Right)
        PickupWords(node->Right, words, spaces);
}

void PickupWords(const TRequestNode* node, TVector<TString>& words, TVector<TString>& spaces) {
    PickupWords_<char>(node, words, spaces);
}

void PickupWords(const TRequestNode* node, TVector<TUtf16String>& words, TVector<TUtf16String>& spaces) {
    PickupWords_<wchar16>(node, words, spaces);
}

void CollectWords(const wchar16* request, TVector<TUtf16String>& words, TVector<TUtf16String>& spaces) {
    THolder<TRequestNode> root(tRequest().Parse(request));
    PickupWords(root.Get(), words, spaces);
}

void CollectWords(const char* request, TVector<TString>& words, TVector<TString>& spaces) {
    const TUtf16String wRequest = CharToWide(request, csYandex);
    THolder<TRequestNode> root(tRequest().Parse(wRequest));
    PickupWords(root.Get(), words, spaces);
}

void tRequest::RunAnalyzer(TNodePtr& root) const {
    Y_ASSERT(root.Get());
    TRequestAnalyzer analyzer;
    root.Reset(analyzer.ProcessTree(root.Release(), ProcessingFlag));
}

using NTokenClassification::TTokenTypes;
using NTokenClassification::IsClassifiedToken;
using NTokenClassification::TTokenClassifiersSingleton;

bool ClassifyTokens(const wchar16* tokenBegin,
                    size_t length,
                    TTokenTypes& TokenTypes)
{
    TokenTypes = 0;
    TokenTypes = TTokenClassifiersSingleton::Classify(tokenBegin, length);

    return IsClassifiedToken(TokenTypes);
}
