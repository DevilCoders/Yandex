#pragma once

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/request/req_node.h>

#include <util/generic/noncopyable.h>

class TLanguageContext;

namespace NRichTreeBuilder {
    inline bool ContainsMultitoken(const TRequestNodeBase& node) {
        Y_ASSERT(IsWordOrMultitoken(node));
        return node.GetPhraseType() != PHRASE_NONE;
    }

    inline bool HasTokenType(const TRichNodePtr& node, ETokenType type) {
        return node->GetSubTokens().size() == 1 && node->GetSubTokens()[0].Type == type;
    }

    inline bool IsWordOrSmth(const TRichNodePtr& node) {
        return IsWordOrMultitoken(*node) && !NRichTreeBuilder::ContainsMultitoken(*node);
    }

    inline bool IsWord(const TRichNodePtr& node) {
        return IsWordOrSmth(node) && HasTokenType(node, TOKEN_WORD);
    }

    inline bool IsNumberX(const TRichNodePtr& node) {
        return IsWordOrSmth(node) && HasTokenType(node, TOKEN_NUMBER);
    }

    inline bool IsMixed(const TRichNodePtr& node) {
        return IsWordOrSmth(node) && HasTokenType(node, TOKEN_MIXED);
    }

    inline bool IsMultitoken(const TRichNodePtr& node) {
        return IsWordOrMultitoken(*node) && NRichTreeBuilder::ContainsMultitoken(*node);
    }

    inline bool IsNonLemmerMultitoken(const TRichNodePtr& node) {
        return IsMultitoken(node) && (node->GetPhraseType() == PHRASE_NUMBERSEQ || node->GetPhraseType() == PHRASE_MARKSEQ);
    }

    inline bool IsLemmerMultitoken(const TRichNodePtr& node) {
        return IsMultitoken(node) && (node->GetPhraseType() == PHRASE_MULTIWORD);
    }

    inline bool IsAttribute(const TRichNodePtr& node) {
        return IsAttribute(*node);
    }

    // Temporary fix. Removes title case for words inside quotes. This is because
    // we want to distinguish case behaviour for !word and "word". Weird.
    inline void FixCaseInQuotes(TRichRequestNode& node) {
        if (IsWord(node) && !!node.WordInfo && fExactWord == node.WordInfo->GetFormType()) {
            node.WordInfo->SubCase(CC_TITLECASE); // ignore case inside quotes
        } else {
            for (size_t i = 0; i < node.Children.size(); i++)
                FixCaseInQuotes(*node.Children[i]);
        }
    }

    //! represents creation context
    class TContext {
    private:
        TFormType FormType;
        TNodeNecessity Necessity;
        bool Quoted;

    public:
        TContext()
            : FormType(fGeneral)
            , Necessity(nDEFAULT)
            , Quoted(false)
        {
        }
        TFormType GetFormType() const {
            if (FormType != fGeneral)
                return FormType;
            if (IsQuoted())
                return fExactWord;
            return fGeneral;
        }
        TNodeNecessity GetNecessity() const {
            return Necessity;
        }
        bool IsQuoted() const {
            return Quoted;
        }
        TFormType GetFormType(const TRequestNode& node) const {
            if (node.FormType == fGeneral)
                return GetFormType();
            return node.FormType;
        }
        TNodeNecessity GetNecessity(const TRequestNode& node) const {
            if (node.Necessity == nDEFAULT)
                return GetNecessity();
            return node.Necessity;
        }
        //! sets prefixes to this context from the given node
        //! @param node     a node from which prefixes are copied
        void Update(const TRequestNode& node) {
            if (node.FormType != fGeneral)
                FormType = node.FormType;
            //            else if (node.IsQuoted())
            //                FormType = fExactWord;

            if (node.IsQuoted())
                Quoted = true;

            if (node.Necessity != nDEFAULT)
                Necessity = node.Necessity;
        }
    };

    void ReplaceMultitokenSubtree(TRichNodePtr& node, TRichNodePtr& parent, const TCreateTreeOptions& options);
    void ProcessMultitoken(const TRequestNode& node, const TContext& ctx, const TProximity& firstProx, bool singleMultitoken, TRichRequestNode& richNode);
    //! @param formType     there is a preference in case of PHRASE_MARKSEQ: fExactWord always used
    void AddMultitokenChildren(const TRequestNode& node, const TContext& ctx, TRichRequestNode& res);

    class TSubTreeCreator;
    class TMultitokenProcessor;
    class TRestrictionsCollector;
    class TLemmerMultitokenDeployer;

    class TTreeCreator {
    public:
        static TRichNodePtr NewNode() {
            return new TRichRequestNode;
        }

        static TRichNodePtr NewNode(const TRequestNodeBase& src, TNodeNecessity contextNecessity, TFormType formType) {
            return new TRichRequestNode(src, contextNecessity, formType);
        }
        static TRichNodePtr NewNode(TFormType formType, TWordJoin leftJoin, TWordJoin rightJoin) {
            return new TRichRequestNode(formType, leftJoin, rightJoin);
        }

    };

    TRichNodePtr CreateTree(const TRequestNode& node);
    TRichNodePtr CreateEmptyTree();

    void PostProcessTree(TRichNodePtr& root, const TCreateTreeOptions& options);
    void Lemmatize(TRichNodePtr& root, const TCreateTreeOptions& options);

    NSearchQuery::TMarkupDataPtr CreateWordWithPrefix(const TRichRequestNode& node, const TUtf16String& prefix);
}
