#pragma once

#include <library/cpp/domtree/iterators.h>
#include <util/stream/output.h>
#include <util/generic/vector.h>

namespace NSegmentator {

struct TFactorNode;

class TPageSegment {
public:
    TFactorNode* First;
    TFactorNode* Last;

public:
    TPageSegment(TFactorNode* first, TFactorNode* last)
        : First(first)
        , Last(last)
    {}

    TPageSegment(TFactorNode* node)
        : TPageSegment(node, node)
    {}

    bool Empty() const {
        return nullptr == First;
    }

    void PrintRawText(IOutputStream& out = Cout) const;
    void PrintPositions(IOutputStream& out = Cout) const;
};

class TPSShortestPathTraverser : NDomTree::IAbstractTraverser<TFactorNode*> {
private:
    TFactorNode* Lca = nullptr;
    TVector<TFactorNode*> FactorNodeQueue;
    TVector<TFactorNode*>::iterator NextNode;
    bool BeforeLca = true;
    const TFactorNode* CurNode = nullptr;
    const TFactorNode* PrevNode = nullptr;

public:
    TPSShortestPathTraverser(const TPageSegment& pageSegment);
    TFactorNode* Next() override;
    bool IsBeforeLca() const;
    bool IsLca() const;
    bool IsAfterLca() const;
    bool IsFirst() const;
    bool IsLast() const;
    const TFactorNode* GetPrev() const;
    const TFactorNode* GetNext() const;
};

void GetPathAndLcaId(const TPageSegment& seg, TVector<TFactorNode*>& path, ui32& lcaId);

}  // NSegmentator
