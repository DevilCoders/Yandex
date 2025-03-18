#include "page_segment.h"

#include "factor_tree.h"

#include <library/cpp/domtree/treetext.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

namespace NSegmentator {

static TString PosOut(TPosting pos) {
    TStringStream out;
    out << GetBreak(pos) << "." << GetWord(pos);
    return out.Str();
}

void GetPathAndLcaId(const TPageSegment& seg, TVector<TFactorNode*>& path, ui32& lcaId) {
    path.clear();
    if (seg.First == nullptr)
        return;

    TFactorNode* lca = TFactorTree::CalcLowestCommonAncestor(seg.First, seg.Last);
    TFactorNode* nextNode = seg.First;
    while (nextNode != lca) {
        path.push_back(nextNode);
        nextNode = nextNode->Parent;
    }
    lcaId = path.size();
    path.push_back(lca);

    TVector<TFactorNode*> beforeLcaNodes;
    nextNode = seg.Last;
    while (nextNode != lca) {
        beforeLcaNodes.push_back(nextNode);
        nextNode = nextNode->Parent;
    }
    for (TVector<TFactorNode*>::reverse_iterator it = beforeLcaNodes.rbegin(); it != beforeLcaNodes.rend(); ++it) {
        path.push_back(*it);
    }

}

// TPSShortestPathTraverser
TPSShortestPathTraverser::TPSShortestPathTraverser(const TPageSegment& pageSegment) {
    Y_ASSERT(!pageSegment.Empty());
    ui32 lcaId = BAD_ID;
    GetPathAndLcaId(pageSegment, FactorNodeQueue, lcaId);
    Y_ASSERT(lcaId != BAD_ID);
    Lca = FactorNodeQueue[lcaId];
    NextNode = FactorNodeQueue.begin();
}

TFactorNode* TPSShortestPathTraverser::Next() {
    PrevNode = CurNode;
    if (NextNode == FactorNodeQueue.end()) {
        CurNode = nullptr;
        return nullptr;
    }
    TFactorNode* curNode = *NextNode++;
    if (curNode == Lca) {
        BeforeLca = false;
    }
    CurNode = curNode;
    return curNode;
}

bool TPSShortestPathTraverser::IsBeforeLca() const {
    return BeforeLca;
}

bool TPSShortestPathTraverser::IsLca() const {
    return CurNode == Lca;
}

bool TPSShortestPathTraverser::IsAfterLca() const {
    return !BeforeLca && CurNode != Lca;
}

bool TPSShortestPathTraverser::IsFirst() const {
    return CurNode == FactorNodeQueue.front();
}

bool TPSShortestPathTraverser::IsLast() const {
    return CurNode == FactorNodeQueue.back();
}

const TFactorNode* TPSShortestPathTraverser::GetPrev() const {
    return PrevNode;
}

const TFactorNode* TPSShortestPathTraverser::GetNext() const {
    if (NextNode == FactorNodeQueue.end()) {
        return nullptr;
    }
    return *NextNode;
}


// TPageSegment
void TPageSegment::PrintPositions(IOutputStream& out) const {
    Y_ASSERT(!Empty());
    out << "[" << PosOut(First->Node->PosBeg()) << ", " << PosOut(Last->Node->PosEnd()) << "]";
}

void TPageSegment::PrintRawText(IOutputStream& out) const {
    Y_ASSERT(!Empty());
    TPSShortestPathTraverser traverser(*this);
    TFactorNode* node = traverser.Next();
    while (nullptr != node) {
        if (traverser.IsFirst() || traverser.IsLast()) {
            out << node->Node->NodeText()->RawTextNormal();
        } else if (traverser.IsBeforeLca()) {
            bool needPrint = false;
            const TFactorNode* prevNode = traverser.GetPrev();
            for (TFactorNode& child : TFactorTree::ChildTraversal(node)) {
                if (needPrint) {
                    out << child.Node->NodeText()->RawTextNormal();
                } else if (&child == prevNode) {
                    needPrint = true;
                }
            }
        } else if (traverser.IsAfterLca()) {
            const TFactorNode* nextNode = traverser.GetNext();
            for (TFactorNode& child : TFactorTree::ChildTraversal(node)) {
                if (&child == nextNode) {
                    break;
                }
                out << child.Node->NodeText()->RawTextNormal();
            }
        }
        node = traverser.Next();
    }
}

}  // NSegmentator
