#include "richtreebuilderaux.h"
#include "subword.h"

#include <kernel/qtree/request/req_node.h>

#include <library/cpp/token/token_util.h>

namespace NRichTreeBuilder {

    static TRichNodePtr MakeRawNode(const TSubWord& subWord, const TRequestNode& node, const TContext& ctx, bool needExactWord) {
        TFormType formType = needExactWord ? fExactWord : ctx.GetFormType(node);
        TRichNodePtr ret = TTreeCreator::NewNode(formType, subWord.GetLeftJoin(), subWord.GetRightJoin());
        ret->Necessity = ctx.GetNecessity(node);
        ret->SetTextBefore(subWord.GetTextBefore());
        ret->SetTextAfter(subWord.GetTextAfter());
        return ret;
    }

    //! constructs a node with single word that is a part of a multitoken
    static TRichNodePtr CreateMultitokenChild(const TSubWord& currentText, const TRequestNode& node, const TContext& ctx,
                                              EHiliteType hilight, bool needExactWord) {
        TRichNodePtr richNode = MakeRawNode(currentText, node, ctx, needExactWord);

        const int wildcard = (currentText.IsOpening() ? WILDCARD_SUFFIX : 0) | (currentText.IsClosing() ? WILDCARD_PREFIX : 0);
        richNode->SetWildCard(static_cast<EWildCardFlags>(node.GetWildCard() & wildcard));

        TCharSpan span(0, currentText.GetLength(), currentText.LastToken().Type); // no prefix
        span.SuffixLen = currentText.LastToken().SuffixLen;
        richNode->SetLeafWord(currentText.GetText(), span);
        richNode->SetHiliteType(hilight);

        return richNode;
    }

    static TRichNodePtr CreateMultitokenParent(const TSubWord& currentText, const TRequestNode& node, const TContext& ctx) {
        TRichNodePtr parent = MakeRawNode(currentText, node, ctx, false);
        parent->SetMultitoken(currentText.GetText(), currentText.ObtainSubTokens(), currentText.GetPhraseType());
        parent->Parens = node.Parens;
        parent->ReverseFreq = node.ReverseFreq;
        parent->SetWildCard(node.GetWildCard());
        return parent;
    }

static TProximity MakeProximity(const TRequestNode& node, const TContext& ctx, const TCharSpan& s) {
    if (node.IsQuoted() || ctx.IsQuoted())
        return TProximity(1, 1, 0, DT_QUOTE);
    else if (s.TokenDelim != TOKDELIM_DOT)
        return TProximity(1, 1, 0, DT_MULTITOKEN);
    else
        return TProximity(0, 0, 2, DT_PHRASE); // make tree invalid in case [a ~ b.c]
}

static void AddWordWithPrefixMarkup(TRichRequestNode* parent, size_t childIndex, const TUtf16String& prefix) {
    // find leftmost lowest child of parent[childIndex]
    while (!parent->Children[childIndex]->Children.empty()) {
        parent = parent->Children[childIndex].Get();
        childIndex = 0;
    }
    TRichRequestNode& word = *parent->Children[childIndex];
    parent->AddMarkup(childIndex, CreateWordWithPrefix(word, prefix));
}

static void AddMultitokenChildren(const TRequestNode& node, const TContext& ctx, size_t first, size_t last, TRichRequestNode& res) {
    const bool markOrNumberSeq = res.GetPhraseType() == PHRASE_MARKSEQ || res.GetPhraseType() == PHRASE_NUMBERSEQ;
    const bool needExactWord = markOrNumberSeq || res.GetWildCard() != WILDCARD_NONE;

    const TTokenStructure& subtoks = node.GetSubTokens();
    Y_ASSERT(first <= last && last < subtoks.size());

    const size_t offset = res.Children.size();
    for (size_t i = first; i <= last; ++i) {
        const EHiliteType hilight = markOrNumberSeq ? (i == first ? HILITE_RIGHT : (i == last ? HILITE_LEFT : HILITE_LEFT_AND_RIGHT)) : HILITE_SELF;

        TRichNodePtr multitoken = CreateMultitokenChild(TSubWord(node, i, i), node, ctx, hilight, needExactWord);
        TProximity prox = (i == first) ? TProximity() : TProximity(1, 1, 0, DT_MULTITOKEN);
        res.Children.Append(multitoken, prox);
    }
    if (!!node.GetPrefix() && first == 0 && last + 1 == subtoks.size()) {
        AddWordWithPrefixMarkup(&res, offset, node.GetPrefix());
    }
}

static TRichNodePtr CreateMultitokenSubTree(const TRequestNode &node, const TContext &ctx, size_t first, size_t last, bool singleMultitoken)  {
    TRichNodePtr parent = CreateMultitokenParent(TSubWord(node, first, last), node, ctx);
    NRichTreeBuilder::AddMultitokenChildren(node, ctx, first, last, *parent);
    if (node.IsQuoted() && singleMultitoken)
        parent->SetQuoted(true);
    return parent;
}


static TUtf16String ExtractPrefixText(const TRequestNode& node, size_t first) {
    if (first == 0 && !!node.GetPrefix()) {
        return node.GetPrefix();
    } else if (node.GetSubTokens()[first].PrefixLen) {
        const TCharSpan& span = node.GetSubTokens()[first];
        return node.GetText().substr(span.Pos - span.PrefixLen, span.PrefixLen);
    } else
        return TUtf16String();
}


    void ProcessMultitoken(const TRequestNode& node, const TContext& ctx, const TProximity& firstProx,
                           bool singleMultitoken, TRichRequestNode& richNode) {

        Y_ASSERT(IsWordOrMultitoken(node) && ContainsMultitoken(node));
        const TTokenStructure& subtokens = node.GetSubTokens();
        size_t first = 0;
        const size_t final = subtokens.size() - 1;

        TProximity prox = firstProx;

        for (size_t i = 0; i < subtokens.size(); ++i) {
            if (i == final || CheckWideTokenReqSplit(subtokens, i)) {
                TSubWord curWord(node, first, i);
                TRichNodePtr multitok = curWord.IsSingleToken()
                    ? CreateMultitokenChild(curWord, node, ctx, HILITE_SELF, false)
                    : CreateMultitokenSubTree(node, ctx, first, i, singleMultitoken);

                richNode.Children.Append(multitok, prox);
                if (i < final)
                    prox = MakeProximity(node, ctx, subtokens[i]);


                // prefix synonym
                const TUtf16String prefix = ExtractPrefixText(node, first);
                if (!prefix.empty())
                    AddWordWithPrefixMarkup(&richNode, richNode.Children.size() - 1, prefix);

                first = i + 1;
            }
        }
    }

    void AddMultitokenChildren(const TRequestNode& node, const TContext& ctx, TRichRequestNode& res) {
        AddMultitokenChildren(node, ctx, 0, res.GetSubTokens().size() - 1, res);
    }

} // NRichTreeBuilder
