#include <kernel/qtree/richrequest/builder/richtreebuilderaux.h>
#include "nodeiterator.h"

#include <util/generic/singleton.h>

#include <kernel/qtree/request/request.h>
#include <kernel/reqerror/reqerror.h>
#include <kernel/qtree/request/req_node.h>
#include <kernel/qtree/request/fixreq.h>


namespace {
    TLangMask FindUsrLang(TRichNodePtr& node) {
        static const wchar16 lang[] = {'l', 'a', 'n', 'g', 0};

        TLangMask ret;
        for (TAttributeIterator iter(node); !iter.IsDone(); ++iter) {
            if (!TUtf16String::compare(iter->GetTextName(), lang) && !!iter->GetText()) {
                const TLanguage* lg = NLemmer::GetLanguageByName(iter->GetText().data() + 1);
                if (lg)
                    ret.SafeSet(lg->Id);
            }
        }
        return ret;
    }

    void RemoveExceedingLeaves(const TCreateTreeOptions& options, TRichNodePtr& root, size_t& cntTokens) {
        if (root->IsLeaf()) {
            cntTokens++;
            return;
        }
        for (size_t i = 0; i < (root->Children.size()); ++i) {
            auto child = root->Children[i];
            size_t cntTokensBefore(cntTokens);
            RemoveExceedingLeaves(options, child, cntTokens);
            if (cntTokens > options.Limits.MaxNumberOfTokens && i) {
                cntTokens = cntTokensBefore;
                root->RemoveChildren(i, root->Children.size());
                return;
            }
        }
    }

    void RemoveExceedingLeaves(const TCreateTreeOptions& options, TRichNodePtr& root) {
        size_t cnt = 0;
        RemoveExceedingLeaves(options, root, cnt);
    }

    void CreateEmptyResults(const TRichNodePtr& root, TCreateRichNodeResults& results) {
        results.Root = root;
        results.RequestLanguages = TLangMask{};
        // UsrLanguages is left unchanged
        results.FoundTokenTypes = 0x0;
    }

    void CreateEmptyResults(const TRichTreePtr& tree, TCreateRichTreeResults& results) {
        results.Tree = tree;
        CreateEmptyResults(tree ? tree->Root : TRichNodePtr{}, results);
    }

    void CreateTree(const TCreateTreeOptions& options, const TRequestNode* root, TCreateRichNodeResults& results) {
        results.Root = nullptr;

        if (!root) {
            results.Root = NRichTreeBuilder::CreateEmptyTree();
            return;
        }

        TRichNodePtr richNode(NRichTreeBuilder::CreateTree(*root));
        Y_ASSERT(richNode.Get());
        NRichTreeBuilder::PostProcessTree(richNode, options);

        if (options.Reqflags & RPF_TRIM_EXTRA_TOKENS)
            RemoveExceedingLeaves(options, richNode);
        results.FoundTokenTypes = 0x0;

        for (TRichNodeIterator iter(richNode); !iter.IsDone(); ++iter) {
            results.FoundTokenTypes |= iter->GetTokenTypes();
        }

        NRichTreeBuilder::Lemmatize(richNode, options);

        // for standalone words, ignore restrictions on highlighting
        if (richNode->WordInfo.Get())
           richNode->SetHiliteType(HILITE_SELF);

        // Apparently UsrLanguages can be
        // used as input? WTF?
        //
        TLangMask uMask = FindUsrLang(richNode);
        uMask |= results.UsrLanguages;
        results.UsrLanguages = uMask;

        // Note that langMask depends on uMask above
        //
        TLangMask langMask = DisambiguateLanguagesTree(richNode.Get(), options.DisambOptions, uMask);
        results.RequestLanguages = langMask;

        if (options.AddDefaultLemmas) {
            for (TAllNodesIterator i(richNode); !i.IsDone(); ++i) {
                if (!!i->WordInfo && i->WordInfo->IsLemmerWord()) {
                    Y_ASSERT(i->WordInfo->AreFormsGenerated());
                    TWordInstanceUpdate(*i->WordInfo).AddDefaultLemma();
                }
            }
        }

        richNode->UpdateKeyPrefix(root->GetKeyPrefix());
        results.Root = std::move(richNode);
    }
} // namespace

// Base CreateRichTree functions
void CreateRichNodeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, TCreateRichNodeResults& results) {
    CreateTree(options, node, results);
}

void CreateRichTreeFromBinaryTree(const TRequestNode* node, const TCreateTreeOptions& options, NSearchRequest::TOperatorsFound foundReqLang, TCreateRichTreeResults& results) {
    CreateRichNodeFromBinaryTree(node, options, results);

    TRichTreePtr res(new NSearchQuery::TRequest);
    res->Root = results.Root;
    res->Softness = node ? node->GetSoftness() : 0;
    res->FoundReqLang = foundReqLang;
    res->FoundTokenTypes = results.FoundTokenTypes;

    results.Tree = res;
}

TRichNodePtr CreateEmptyRichNode() {
    return NRichTreeBuilder::CreateEmptyTree();
}

TRichTreePtr CreateEmptyRichTree() {
    TRichTreePtr res(new NSearchQuery::TRequest);
    res->Root = CreateEmptyRichNode();
    return res;
}

void CreateRichTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt, TCreateRichTreeResults& results) {
    if (!query) {
        CreateEmptyResults(CreateEmptyRichTree(), results);
        return;
    }

    const TLengthLimits* limits = treeOpt.Reqflags & RPF_TRIM_EXTRA_TOKENS ? &treeOpt.Limits : nullptr;
    THolder<TRequestNode> treeRoot;
    NSearchRequest::TOperatorsFound foundReqLang;
    if (treeOpt.AttrListTReq)
        treeRoot.Reset(CreateBinaryTree(query, treeOpt.AttrListTReq, treeOpt.Reqflags, limits, treeOpt.TokenizerVersion, &foundReqLang).first);
    else
        treeRoot.Reset(CreateBinaryTree(query, treeOpt.AttrListChar, treeOpt.Reqflags, limits, treeOpt.TokenizerVersion, &foundReqLang).first);
    CreateRichTreeFromBinaryTree(treeRoot.Get(), treeOpt, foundReqLang, results);
}

void CreateRichNode(const TUtf16String& query, const TCreateTreeOptions& treeOpt, TCreateRichNodeResults& results) {
    if (!query) {
        CreateEmptyResults(CreateEmptyRichNode(), results);
        return;
    }

    const TLengthLimits* limits = treeOpt.Reqflags & RPF_TRIM_EXTRA_TOKENS ? &treeOpt.Limits : nullptr;
    THolder<TRequestNode> treeRoot;
    if (treeOpt.AttrListTReq)
        treeRoot.Reset(CreateBinaryTree(query, treeOpt.AttrListTReq, treeOpt.Reqflags, limits, treeOpt.TokenizerVersion).first);
    else
        treeRoot.Reset(CreateBinaryTree(query, treeOpt.AttrListChar, treeOpt.Reqflags, limits, treeOpt.TokenizerVersion).first);
    CreateRichNodeFromBinaryTree(treeRoot.Get(), treeOpt, results);
}

TRichTreePtr TryCreateRichTree(const TUtf16String& query, const TCreateTreeOptions& treeOpt) {
    try {
        return CreateRichTree(query, treeOpt);
    } catch (const TError&) {
        return nullptr;
    }
}

TRichNodePtr TryCreateRichNode(const TUtf16String& query, const TCreateTreeOptions& treeOpt) {
    try {
        return CreateRichNode(query, treeOpt);
    } catch (const TError&) {
        return nullptr;
    }
}

