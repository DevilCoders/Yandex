#include "readability.h"

namespace NSnippets {

TSymbolsStat::TSetType& TSymbolsStat::CodePage(const TString& name) {
    return Info[name].Set;
}
int TSymbolsStat::IsMatched(const TString& name) const {
    auto it = Info.find(name);
    return (it == Info.end()) ? 0 : it->second.IsMatched;
}
void TSymbolsStat::Test(wchar16 c) {
    for (auto& it : Info) {
        TStat& stat = it.second;
        if (stat.IsMatched == 0 && stat.Set.Check(c)) {
            stat.IsMatched = 1;
        }
    }
}

}

