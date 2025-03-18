#include "segmentator_handler.h"

namespace NSegmentator {
    // INumeratorHandler interface
    void TSegmentator2015Handler::OnTextStart(const IParsedDocProperties* prop)
    {
        TreeBuilderHandler->OnTextStart(prop);
    }

    void TSegmentator2015Handler::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* ze,
        const TNumerStat& stat)
    {
        TreeBuilderHandler->OnMoveInput(chunk, ze, stat);
    }

    void TSegmentator2015Handler::OnTextEnd(const IParsedDocProperties*, const TNumerStat&)
    {
        Segmentator.InitDomTreePtr(TreeBuilderHandler->GetTree());
    }

    TSegmentator2015Handler::TSegmentator2015Handler(const TString& url)
        : TreeBuilderHandler(NDomTree::TreeBuildingHandler())
        , Segmentator(url)
    {
    }

    const TVector<TPageSegment>& TSegmentator2015Handler::GetOrCalcSegments() {
        return Segmentator.GetOrCalcSegments();
    }
}

