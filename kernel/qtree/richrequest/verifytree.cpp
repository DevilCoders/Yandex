#include "richnode.h"

namespace {

class TNodeInfo {
public:
    TNodeInfo(const TRichRequestNode& node, const TNodeInfo* parentInfo)
    : Node(node)
    , Prnt(parentInfo)
    {
    }

    bool IsRoot() const {
        return !Prnt;
    }

    const TNodeInfo& Parent() const {
        Y_ASSERT(Prnt);
        return *Prnt;
    }

    const TNodeSequence& Children() const {
        return Node.Children;
    }

    size_t ChildrenNumber() const {
        return Node.Children.size();
    }

    const TRichRequestNode& GetChild(size_t i) const {
        if (!Node.Children[i])
            ythrow TCorruptedTreeException() << "NULL child";
        return *Node.Children[i];
    }

    size_t MiscOpsNumber() const {
        return Node.MiscOps.size();
    }

    const TRichRequestNode& GetMiscOp(size_t i) const {
        if (!Node.MiscOps[i])
            ythrow TCorruptedTreeException() << "NULL tree in MiscOps";
        return *Node.MiscOps[i];
    }

    const NSearchQuery::TMarkup& Markup() const {
        return Node.Markup();
    }

    bool IsNormalLeaf() const {
        return IsWord(Node) || IsAttribute(Node);
    }

    bool IsMiscLeaf() const {
        return IsZone(Node) || Node.OpInfo.Op == oRestrictByPos;
    }

    bool IsAndOp() const {
        return ::IsAndOp(Node);
    }

    bool IsUnaryOp() const { // should have one child (if any)
        return (GetOp() == oAndNot) || (GetOp() == oRefine) || (GetOp() == oRestrDoc) || (GetOp() == oZone);
    }

    bool IsBinaryOp() const { // should have at least two children
        return GetOp() == oOr;
    }

    bool IsSimpleOp() const {
        return (GetOp() == oAnd) || (GetOp() == oOr);
    }

    bool IsMiscOp() const {
        return IsUnaryOp();
    }

    bool HasWordInfo() const {
        return !!Node.WordInfo;
    }

    TString GetOpString() const {
        return ToString(GetOp());
    }

private:
    TOperType GetOp() const {
        return Node.Op();
    }

private:
    const TRichRequestNode& Node;
    const TNodeInfo* Prnt;
};

static bool AllParentsAreBinary(const TNodeInfo& ni) {
    const TNodeInfo *pi = &ni;
    while (!pi->IsRoot()) {
        pi = &pi->Parent();
        if (!pi->IsBinaryOp())
            return false;
    }
    return true;
}

template <bool InMiscOps, bool InSynonym>
class TNodeVerifier {
public:
    TNodeVerifier(const TRichRequestNode& node, const TNodeInfo* parentInfo)
    : Ni(node, parentInfo)
    {
        VerifyNode();
    }

private:
    void VerifyNode() {
        if (!Ni.Children().VerifySize())
            ythrow TCorruptedTreeException() << "NodeSequence is corrupted";

        VerifyMarkupCoordinates();

        VerifyIfSynonym();
        VerifyIfMiscOp();

        if (!Ni.ChildrenNumber())
            VerifyLeaf();
        else
            VerifyInternalNode();

        VerifySynonyms();

        VerifyMiscOps();

        for (size_t i = 0; i < Ni.ChildrenNumber(); ++i) {
            TNodeVerifier<InMiscOps, InSynonym>(Ni.GetChild(i), &Ni);
        }
    }

    void VerifyIfSynonym() {
        if (!InSynonym)
            return;
        if (Ni.MiscOpsNumber())
            ythrow TCorruptedTreeException() << "Synonym should have no miscops";

        if (!Ni.HasWordInfo() && !(Ni.IsRoot() && Ni.IsAndOp()))
            ythrow TCorruptedTreeException() << "Tree in synonym should be a simple phrase";
    }

    void VerifyIfMiscOp() {
        if (!InMiscOps)
            return;
        if (AllParentsAreBinary(Ni)) { // or Root
            if (!Ni.IsMiscOp() && !Ni.IsMiscLeaf() && !Ni.IsBinaryOp())
                ythrow TCorruptedTreeException() << "MiscOp should be a special operation";
        } else {
            if (!Ni.IsAndOp() && !Ni.IsNormalLeaf() && !Ni.IsBinaryOp())
                ythrow TCorruptedTreeException() << "Special operation should have ordinary children";
        }
    }

    void VerifyMarkupCoordinates() const {
        // special case for empty Children: only [0,0] markup is allowed
        const size_t rangeLast = Ni.ChildrenNumber() == 0 ? 0 : Ni.ChildrenNumber() - 1;

        for (NSearchQuery::TAllMarkupConstIterator i(Ni.Markup()); !i.AtEnd(); ++i) {
            if (i->Range.Beg > i->Range.End || i->Range.End > rangeLast)
                ythrow TCorruptedTreeException() << "bad markup coordinates";
        }
    }

    void VerifySynonyms() const {
        VerifySynonymsInt<TSynonym>();
        VerifySynonymsInt<TTechnicalSynonym>();
    }

    template <class TSynonymType>
    void VerifySynonymsInt() const {
        for (NSearchQuery::TForwardMarkupIterator<TSynonymType, true> it(Ni.Markup()); !it.AtEnd(); ++it) {
            if (!it.GetData().SubTree)
                ythrow TCorruptedTreeException() << "NULL tree in synonym";
            TNodeVerifier<false, true>(*it.GetData().SubTree, nullptr);
        }
    }

    void VerifyMiscOps() const {
        for (size_t i = 0; i < Ni.MiscOpsNumber(); ++i) {
            TNodeVerifier<true, false>(Ni.GetMiscOp(i), nullptr);
        }
    }

    void VerifyLeaf() const {
        if (!Ni.IsNormalLeaf() && !(InMiscOps && Ni.IsMiscLeaf()))
            ythrow TCorruptedTreeException() << "leaf should be a word or an attribute or, if miscops, a zone";
        if (!(Ni.IsMiscLeaf() ^ Ni.HasWordInfo()))
            ythrow TCorruptedTreeException() << "leaf, except zones, should have WordInfo, zone - must not";
    }

    void VerifyInternalNode() const {
        if (Ni.HasWordInfo())
            ythrow TCorruptedTreeException() << "internal vertex should have no wordinfo";

        CheckAllowedOp();
        CheckArity();
    }

    void CheckArity() const {
        if (Ni.IsBinaryOp()) {
            if (Ni.ChildrenNumber() < 2)
                ythrow TCorruptedTreeException() << Ni.GetOpString() << " node should have at least two children, but there is " << Ni.ChildrenNumber();
        } else if (Ni.IsUnaryOp()) {
            if (Ni.ChildrenNumber() != 1)
                ythrow TCorruptedTreeException() << Ni.GetOpString() << " node should have one child, but there is " << Ni.ChildrenNumber();
        }
    }

    void CheckAllowedOp() const {
        if (InMiscOps) {
            if (!Ni.IsSimpleOp() && !Ni.IsMiscOp())
                ythrow TCorruptedTreeException() << Ni.GetOpString() << " operation not allowed in internal misc node";
        } else {
            if (!Ni.IsSimpleOp())
                ythrow TCorruptedTreeException() << Ni.GetOpString() << " operation not allowed in internal node";
        }
    }
private:
    const TNodeInfo Ni;
};

} // namespace

void TRichRequestNode::VerifyConsistency() const {
    TNodeVerifier<false, false>(*this, nullptr);
}
