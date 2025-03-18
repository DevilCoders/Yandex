#pragma once

#include "segmentator.h"

#include <library/cpp/domtree/numhandler.h>
#include <library/cpp/numerator/numerate.h>

namespace NSegmentator {
    class TSegmentator2015Handler :  public INumeratorHandler {
    private:
        NDomTree::TNumHandlerPtr TreeBuilderHandler;
        TSegmentator Segmentator;

    private:
        // INumeratorHandler interface
        void OnTextStart(const IParsedDocProperties*) override;

        void OnMoveInput(const THtmlChunk&, const TZoneEntry*, const TNumerStat&) override;

        void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) override;

    public:
        TSegmentator2015Handler(const TString& url);
        const TVector<TPageSegment>& GetOrCalcSegments();
    };
}
