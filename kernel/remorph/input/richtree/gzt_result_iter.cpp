#include "gzt_result_iter.h"

namespace NReMorph {

namespace NRichNode {

bool IsDividedByComma(const TGztResultPosition& gztItem) {
    if (gztItem.Size() == 1)
        return false;

    const TRichRequestNode::TNodesVector& parent = gztItem->GetOriginalPhrase();
    if (gztItem.GetStopIndex() > 0) {
        for (size_t i = gztItem.GetStartIndex(); i < gztItem.GetStopIndex()-1; i++) {
            if (i >= parent.size())
                return true;
            TRichNodePtr node = parent[i];
            if (node->GetPunctAfter().find(',') != TUtf16String::npos)
                return true;
        }
    }
    return false;
}

TGztResultIter::TGztResultIter(const TGztResults& gztResults, const TConstNodesVector& nodes,
    TGztResultFilterFunc filter, const TVector<size_t>& offsetMapping)
    : Filter(filter)
    , OffsetMapping(offsetMapping)
    , EmptyMapping(offsetMapping.empty())
{
    gztResults.FindByContentNodes(nodes, Results);
    Reset();
}

TGztResultIter::TGztResultIter(const TVector<TGztResultPosition>& results, TGztResultFilterFunc filter,
    const TVector<size_t>& offsetMapping)
    : Results(results)
    , Filter(filter)
    , OffsetMapping(offsetMapping)
    , EmptyMapping(offsetMapping.empty())
{
    Reset();
}

void TGztResultIter::Reset() {
    Iter = Results.begin();
    while (Iter != Results.end() && nullptr != Filter && Filter(*Iter)) {
        ++Iter;
    }
}

} // NRichNode

} // NReMorph
