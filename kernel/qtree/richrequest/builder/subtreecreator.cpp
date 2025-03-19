#include "richtreebuilderaux.h"

#include <util/generic/yexception.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <kernel/qtree/request/req_node.h>

namespace NRichTreeBuilder {

typedef TVector<NSearchQuery::TMarkupDataPtr> TSynVector;

static TRichNodePtr CreateSubTree(const TRequestNode& node, const TContext& context, TSynVector& syn);

template<typename T>
static void AppendOrSwap(TVector<T>& to, TVector<T>& from) {
    if (from.empty()) // just to keep my serenity
        return;
    if (to.empty())
        to.swap(from);
    else
        to.insert(to.end(), from.begin(), from.end());
}

static void AddMarkup(TRichRequestNode& parent, const TRichRequestNode& child, const TSynVector& syn) {
    for (size_t i = 0; i < syn.size(); ++i)
        parent.AddMarkup(child, syn[i]);
}

static void AddMarkup(TRichRequestNode& parent, const TSynVector& syn) {
    for (size_t i = 0; i < syn.size(); ++i)
        parent.AddMarkup(syn[i]);
}

class TRestrictionsCollector: private TNonCopyable {
private:
    TVector<TRichNodePtr> Restrictions;
public:
    const TRequestNode* Node;
    TContext Context;
    TRichNodePtr RichNode;
public:
    TRestrictionsCollector(const TRequestNode& node, const TContext& context)
        : Node (&node)
        , Context(context)
    {
        CollectRestrictions();

        RichNode = ObtainCurrentNode();
        RichNode->MiscOps.swap(Restrictions);
    }

private:
    TRichNodePtr ObtainCurrentNode() const {
        return TTreeCreator::NewNode(*Node, Context.GetNecessity(), Context.GetFormType(*Node));
    }
    void CollectRestrictions();
    bool DoCollectRestriction();
    void TurnLeft();
    void StoreLeftSubTreeTurnRight();
    void StoreRightSubTreeTurnLeft();
    void AddRestriction();
    void StoreSubTree(TRequestNode* node);
};

void TRestrictionsCollector::CollectRestrictions() {
    do {
        if (!Node)
            ythrow yexception() << "CollectRestrictions failed.";
        Context.Update(*Node);
    } while (DoCollectRestriction());
}

bool TRestrictionsCollector::DoCollectRestriction() {
   switch (Node->Op()) {
        case oRestrictByPos:
            Y_ASSERT(!Node->Right && Node->Left);
            TurnLeft();
            return true;
        case oZone:
            // the Left node can be an attribute node or NULL
            if (Node->Right)
                StoreLeftSubTreeTurnRight();
            else // requests with empty zone text 'zone#attr="a b"' and 'zone#attr=(a b)'
                TurnLeft();
            return true;
        case oAndNot:
        case oRestrDoc:
        case oRefine:
            Y_ASSERT(Node->Right && Node->Left);
            StoreRightSubTreeTurnLeft();
            return true;
        default:
            return false;
    }
}

void TRestrictionsCollector::TurnLeft() {
    AddRestriction();
    Node = Node->Left;
}

void TRestrictionsCollector::StoreLeftSubTreeTurnRight() {
    AddRestriction();
    StoreSubTree(Node->Left);
    Node = Node->Right;
}

void TRestrictionsCollector::StoreRightSubTreeTurnLeft() {
    AddRestriction();
    StoreSubTree(Node->Right);
    Node = Node->Left;
}

void TRestrictionsCollector::AddRestriction() {
    Restrictions.push_back(ObtainCurrentNode());
}

void TRestrictionsCollector::StoreSubTree(TRequestNode* node) {
    if (!node)
        return;
    TRichNodePtr parent = Restrictions.back();
    TSynVector syn;
    TRichNodePtr child = CreateSubTree(*node, Context, syn);
    parent->Children.Append(child);
    AddMarkup(*parent, *child, syn);
}

//! a recursive class that creates a rich tree node from a given subtree
//! @param node     the root of the subtree
class TSubTreeCreator: private TNonCopyable {
private:
    const TRequestNode& Node;
    const TContext& Context;
    TRichRequestNode& RichNode;
private:
    //! collects children of a node into the given collection
    void CollectChildren();
    TRichNodePtr CollectCompositeMultitokenChild(const TRequestNode& child, const TContext &ctx) const;

    void AddChildSubTree(const TRequestNode& node, const TProximity& prox) {
        TSynVector syn;
        TRichNodePtr child(CreateSubTree(node, Context, syn));
        RichNode.Children.Append(child, prox);
        AddMarkup(RichNode, *child, syn);
    }

    void ProceedMultitoken();
    void ProceedWord();
    void ProceedAttribute();
    void ProceedOperation();
public:
    TSubTreeCreator(const TRestrictionsCollector& restr, TSynVector& syn);
    static TRichNodePtr CreateEmptyTree() {
        return TTreeCreator::NewNode();
    }
};

static TDistanceType PhraseType2DistanceType(TPhraseType phraseType){
    switch (phraseType){
        case PHRASE_NONE:   return DT_NONE;

        case PHRASE_MARK:
        case PHRASE_MARKSEQ:
        case PHRASE_MULTIWORD:
        case PHRASE_NUMBERSEQ:
        case PHRASE_FLOAT:  return DT_MULTITOKEN;

        case PHRASE_PHRASE: return DT_PHRASE;
        case PHRASE_USEROP: return DT_USEROP;
        default:            return DT_NONE;
    }
}

static bool IsSubTree(const TRequestNode& node, const TRequestNode& child) {
    return (node.Op() == child.Op())
        && !child.IsEnclosed()
        && (child.GetPhraseType() != PHRASE_MULTIWORD ||
            node.GetPhraseType() == PHRASE_MULTIWORD);
}

static void CollectSubTree(const TRequestNode& node, TVector<const TRequestNode*>& nodes, TVector<TProximity>& proxes) {
    if (!node.Left || !node.Right)
        return;

    if (IsSubTree(node, *node.Left))
        CollectSubTree(*node.Left, nodes, proxes);
    else
        nodes.push_back(node.Left);

    TDistanceType dt = IsQuote(node) ? DT_QUOTE : PhraseType2DistanceType(node.GetPhraseType());
    proxes.push_back(TProximity(node.OpInfo.Lo, node.OpInfo.Hi, node.OpInfo.Level, dt));

    if (IsSubTree(node, *node.Right))
        CollectSubTree(*node.Right, nodes, proxes);
    else
        nodes.push_back(node.Right);
}


void TSubTreeCreator::CollectChildren() {
    TVector<const TRequestNode*> nodes;
    TVector<TProximity> proxes;
    CollectSubTree(Node, nodes, proxes);

    for (size_t i = 0; i < nodes.size(); ++i) {
        Y_ASSERT(nodes[i]);
        const TRequestNode& child = *nodes[i];
        TProximity prox = i > 0 ? proxes[i - 1] : TProximity();

        if (!IsWordOrMultitoken(child) || !ContainsMultitoken(child)) {
            // any type of node can be in the nodes collection so call to CreateSubTree
            AddChildSubTree(child, prox);
            continue;
       }

        TContext ctx = Context;
        ctx.Update(child);
        const bool singleMultitoken = (child.GetPhraseType() != PHRASE_MULTISEQ);
        if ((child.IsQuoted() || child.Parens || RichNode.Op() != oAnd) && !singleMultitoken) { // composite multitoken can't have text: a-b/c+3@d1
            TRichNodePtr mt = CollectCompositeMultitokenChild(child, ctx);
            RichNode.Children.Append(mt, prox);
        } else {
            ProcessMultitoken(child, ctx, prox, singleMultitoken, RichNode);
        }
    }
}

TRichNodePtr TSubTreeCreator::CollectCompositeMultitokenChild(const TRequestNode& child, const TContext &ctx) const {
    TRichNodePtr parent = TTreeCreator::NewNode(ctx.GetFormType(child), WORDJOIN_UNDEF, WORDJOIN_UNDEF);
    parent->SetPhraseType(child.IsQuoted() ? PHRASE_USEROP : PHRASE_PHRASE);
    parent->OpInfo = DefaultQuoteOpInfo;
    parent->Necessity = ctx.GetNecessity();
    parent->Parens = false;

    ProcessMultitoken(child, ctx, TProximity(), false, *parent);

    if (child.IsQuoted())
        parent->SetQuoted(true);
    else if (RichNode.Op() == oAnd || ctx.GetNecessity() != nDEFAULT)
        parent->Parens = true;
    return parent;
}

TSubTreeCreator::TSubTreeCreator(const TRestrictionsCollector& restr, TSynVector& syn)
    : Node(*restr.Node)
    , Context(restr.Context)
    , RichNode(*restr.RichNode)
{
   if (IsWordOrMultitoken(RichNode)) {
        if (ContainsMultitoken(RichNode)) { // create oAnd instead of a leaf
            ProceedMultitoken();
        } else {
            ProceedWord();
            if (!!Node.GetPrefix()) {
                syn.push_back(CreateWordWithPrefix(RichNode, Node.GetPrefix()));
            }
        }
    } else if (IsAttribute(RichNode)) {
        ProceedAttribute();
    } else {
        Y_ASSERT(RichNode.Op() == oAnd || RichNode.Op() == oOr);
        ProceedOperation();
    }
}

void TSubTreeCreator::ProceedMultitoken() {
    RichNode.SetTextAfter(TUtf16String());
    RichNode.SetTextBefore(TUtf16String());
    const TPhraseType phraseType = RichNode.GetPhraseType();
    Y_ASSERT(phraseType == PHRASE_MULTIWORD || phraseType == PHRASE_MARKSEQ || phraseType == PHRASE_NUMBERSEQ || phraseType == PHRASE_MULTISEQ);
    const bool singleMultitoken = (phraseType != PHRASE_MULTISEQ);
    if (!singleMultitoken) { // composite multitoken can't have text: a-b/c+3@d1
        RichNode.SetPhrase(Context.IsQuoted() ? PHRASE_USEROP : PHRASE_PHRASE);
        RichNode.OpInfo = Context.IsQuoted() ? DefaultQuoteOpInfo : DefaultPhraseOpInfo;
        // - actually in case of A.S.Pushkin it should not be &/(1 1) USEROP but it can't be && PHRASE
        //   because in case of request [a ~ b.c.d] there will be invalid priorities between operators in the tree
        //   it's a problem...
        // - to be compatible with current wizards PHRASE_PHRASE is used instead of PHRASE_USEROP
    } else { // can have text: a-b-c, a1b2c3, 1.2.3
        RichNode.OpInfo = DefaultQuoteOpInfo;
    }

    if (singleMultitoken) {
        AddMultitokenChildren(Node, Context, RichNode);
    } else {
        ProcessMultitoken(Node, Context, TProximity(), singleMultitoken, RichNode);
    }
}

void TSubTreeCreator::ProceedWord() {
    Y_ASSERT(RichNode.GetSubTokens().size() == 1);
}

void TSubTreeCreator::ProceedAttribute() {
}

void TSubTreeCreator::ProceedOperation() {
    CollectChildren();
}

static TRichNodePtr CreateSubTree(const TRequestNode& node, const TContext& context, TSynVector& syn) {
    TRestrictionsCollector rc(node, context);
    if (rc.Node->Op() == oWeakOr) {
        TSynVector synLeft;
        TSynVector synRight;
        TRichNodePtr left = CreateSubTree(*rc.Node->Left, rc.Context, synLeft);
        AppendOrSwap(left->MiscOps, rc.RichNode->MiscOps);

        AppendOrSwap(syn, synLeft);
        AppendOrSwap(syn, synRight);

        TRichNodePtr right = CreateSubTree(*rc.Node->Right, rc.Context, synRight);
        syn.push_back(NSearchQuery::TMarkupDataPtr(new TSynonym(right, TE_WEAKOR)));

        return left;
    }

    // if request node contains multitoken then oAnd node is created instead of a leaf
    TSubTreeCreator stc(rc, syn);
    return rc.RichNode;
}

TRichNodePtr CreateTree(const TRequestNode& node) {
    TSynVector syn;
    TRichNodePtr root = CreateSubTree(node, TContext(), syn);
    AddMarkup(*root, syn);
    return root;
}

TRichNodePtr CreateEmptyTree() {
    return TSubTreeCreator::CreateEmptyTree();
}

static TRichNodePtr MakePrefixNode(const TUtf16String& prefix) {
    Y_ASSERT(prefix.size() == 1);
    TRichNodePtr prefixNode(TTreeCreator::NewNode(fGeneral, WORDJOIN_DEFAULT, WORDJOIN_NODELIM));
    wchar16 buf[PUNCT_PREFIX_BUF];
    EncodeWordPrefix(prefix[0], buf);
    prefixNode->SetLeafWord(buf, TCharSpan(0, PUNCT_PREFIX_LEN));
    return prefixNode;
}

NSearchQuery::TMarkupDataPtr CreateWordWithPrefix(const TRichRequestNode& node, const TUtf16String& prefix) {
    TRichNodePtr andOp(TTreeCreator::NewNode(fGeneral, WORDJOIN_DEFAULT, WORDJOIN_DEFAULT));

    andOp->Children.Append(MakePrefixNode(prefix));

    TProximity zeroProx(0, 0);
    bool hasChildren = IsAndOp(node) && !node.Children.empty();
    const TRichRequestNode* refNode = hasChildren ? node.Children[0].Get() : &node;
    TRichNodePtr word(TTreeCreator::NewNode(*refNode, nDEFAULT, fExactWord));
    andOp->Children.Append(word, zeroProx);

    if (hasChildren) {
        for (size_t i = 1; i < node.Children.size(); ++i) {
            word = TTreeCreator::NewNode(*node.Children[i], nDEFAULT, fExactWord);
            andOp->Children.Append(word, node.Children.ProxBefore(i));
        }
    }

    return NSearchQuery::TMarkupDataPtr(new TTechnicalSynonym(andOp, TE_WORD_WITH_PREFIX));
}

} // NRichTreeBuilder
