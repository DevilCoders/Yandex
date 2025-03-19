#include "detectors.h"
#include "lemmas_collector.h"

void TTitleDetector::Detect(ui32 position, const TString& /*lemma*/) {
    if (WordIsFound)           // some kind of optimization
        return;
    WordIsFound = (BEST_RELEV == TWordPosition::GetRelevLevel(position));
}

void TPositionDetector::Detect(ui32 curWordPos, const TString& /*lemma*/) {
    if (!CurPositionIsCatched(curWordPos))
        return;

    WordPos = curWordPos;
}

bool TPositionDetector::CurPositionIsCatched(ui32 curWordPos) {
    // havn't prev detector - it means catch in any cases
    if (!PrevDetector)
        return true;

    ui32 prevWordPos = PrevDetector->GetCatchedPos();
    if (prevWordPos == UndefinedPos || (curWordPos - prevWordPos) != Distance)
        return false;

    return true;
}


