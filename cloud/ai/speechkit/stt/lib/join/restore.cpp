#include "restore.h"

void AddReversedToAns(const TText& added, TText& ans) {
    TText addedCopy = added;
    addedCopy.Reverse();
    for (TString& d : addedCopy.Words) {
        ans.Add(d);
    }
}
