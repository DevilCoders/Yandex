#include "richtreebuilderaux.h"
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/search_daemon_iface/reqtypes.h>

namespace NRichTreeBuilder {

static bool Lemmatize_(TRichNodePtr& node, TRichNodePtr& parent, const TCreateTreeOptions& options) {
    if (IsWord(node)) {
        if (options.NoLemmerNodes) {
            node->WordInfo.Reset(TWordNode::CreateNonLemmerNode(node->GetText(), node->GetFormType()).Release());
            node->SetStopWordHiliteMode();
        } else {
            const TCharSpan tokSpan = node->GetTokSpan();
            if (tokSpan.PrefixLen) {
                TCharSpan noPrefixSpan(tokSpan);
                noPrefixSpan.Pos -= tokSpan.PrefixLen;
                noPrefixSpan.PrefixLen = 0;
                const wchar16* text = node->GetText().c_str();
                node->WordInfo.Reset(TWordNode::CreateLemmerNode(TUtf16String(text + tokSpan.Pos, tokSpan.Len + tokSpan.SuffixLen),
                    noPrefixSpan, node->GetFormType(), options.Lang, options.GenerateForms).Release());
            } else {
                node->WordInfo.Reset(TWordNode::CreateLemmerNode(node->GetText(), tokSpan, node->GetFormType(), options.Lang, options.GenerateForms).Release());
            }
            if (parent->GetPhraseType() == PHRASE_MARKSEQ) // see assigning needExactWord in process_multitoken.cpp
                node->WordInfo->SubCase(CC_TITLECASE); // backward compatible behavior for marks: vs2008 == Vs2008 (it makes vs-2008 == Vs-2008 as well)
            node->SetStopWordHiliteMode();
        }
    } else if (IsNumberX(node)) {
        const TCharSpan span = node->GetTokSpan();
        const wchar16* text = node->GetText().c_str();
        node->WordInfo.Reset(TWordNode::CreateTextIntegerNode(TUtf16String(text + span.Pos, span.Len + span.SuffixLen), node->GetFormType(), options.Reqflags & RPF_NUMBERS_STYLE_NO_ZEROS).Release());
        node->SetStopWordHiliteMode();
    } else if (IsLemmerMultitoken(node)) {
        ReplaceMultitokenSubtree(node, parent, options);
        Y_ASSERT(node->GetPhraseType() != PHRASE_MULTIWORD);
        return false;
    } else if (IsAttribute(node)) {
        if (!node->GetAttrValueHi() && node->OpInfo.CmpOper == cmpEQ && !!node->GetText() && node->GetText().at(0) != '\"') {
            node->WordInfo.Reset(TWordNode::CreateAttributeIntegerNode(node->GetText(), node->GetFormType()).Release());
        } else {
            node->WordInfo.Reset(TWordNode::CreateEmptyNode().Release());
        }

        node->SetStopWordHiliteMode();
    } else if (IsMixed(node)) {
        node->WordInfo.Reset(TWordNode::CreateNonLemmerNode(node->GetText(), node->GetFormType()).Release());
        node->SetStopWordHiliteMode();
    }
    Y_ASSERT(!IsWordOrMultitoken(*node) && !IsAttribute(*node) || node->WordInfo.Get());
    Y_ASSERT(node->GetPhraseType() != PHRASE_MULTIWORD);
    return true;
}


static void LemmatizeInt(TRichNodePtr& root, const TCreateTreeOptions& options);
static void LemmatizeInt(TRichNodePtr& node, TRichNodePtr& parent, const TCreateTreeOptions& options) {
    if (Lemmatize_(node, parent, options)) {
        for (size_t i = 0; i < node->Children.size(); ++i)
            LemmatizeInt(node->Children.MutableNode(i), node, options);
    }

    for (size_t i = 0; i < node->MiscOps.size(); ++i) {
        LemmatizeInt(node->MiscOps[i], options);
        CollectSubTree(node->MiscOps[i]);
    }

    for (NSearchQuery::TForwardMarkupIterator<TSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i) {
        LemmatizeInt(i.GetData().SubTree, options);
        CollectSubTree(i.GetData().SubTree, true);
    }

    for (NSearchQuery::TForwardMarkupIterator<TTechnicalSynonym, false> i(node->MutableMarkup()); !i.AtEnd(); ++i) {
        LemmatizeInt(i.GetData().SubTree, options);
        CollectSubTree(i.GetData().SubTree, true);
    }

    if (node->IsQuoted())
        FixCaseInQuotes(*node);

    if (parent->IsChildCoveredByMarkup<TSynonym, NSearchQuery::TSynonymTypeCheck>(*node, NSearchQuery::TSynonymTypeCheck(TE_MARK)))
        FixCaseInQuotes(*node);
}

static inline void LemmatizeInt(TRichNodePtr& root, const TCreateTreeOptions& options) {
    LemmatizeInt(root, root, options);
}

void Lemmatize(TRichNodePtr& root, const TCreateTreeOptions& options) {
    LemmatizeInt(root, options);
    CollectSubTree(root);
}
} // NRichTreeBuilder
