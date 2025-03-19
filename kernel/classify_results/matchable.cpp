#include <library/cpp/binsaver/bin_saver.h>
#include <util/charset/wide.h>
#include <util/system/yassert.h>
#include "matchable.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TMatchable
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TMatchable::IsMatch(const TUtf16String& what, int* id) const {
    TWtrokaAndIds::const_iterator isSimple = SimplePatterns.find(what);
    if (isSimple != SimplePatterns.end()) {
        if (id)
            *id = isSimple->second;
        return true;
    }
    for (size_t i = 0; i < RxPatterns.size(); ++i) {
        if (RxPatterns[i].first.IsMatch(what)) {
            if (id)
                *id = RxPatterns[i].second;
            return true;
        }
    }
    return false;
}

int TMatchable::operator& (IBinSaver& f) {
    f.Add(2, &SimplePatterns);
    f.Add(3, &RxPatterns);
    return 0;
}

void TMatchable::AddSimple(const TUtf16String& templ, const int id) {
    if (SimplePatterns.contains(templ)) {
        if (SimplePatterns[templ] != id)
            fprintf(stderr, "error: Same marker with different id: %s\n", WideToUTF8(templ).data());
        else
            fprintf(stderr, "warning: Duplicate marker: %s\n", WideToUTF8(templ).data());
    }
    SimplePatterns[templ] = id;
}

void TMatchable::AddRx(const TUtf16String& templ, const int id) {
    if (RxPatterns.empty()) {
        RxPatterns.push_back(TRxAndId(TRxMatcher(), id));
    } else {
        Y_VERIFY(RxPatterns.back().second <= id);
        if (RxPatterns.back().second != id)
             RxPatterns.push_back(TRxAndId(TRxMatcher(), id));
    }

    TRxMatcher &dst = RxPatterns.back().first;
    dst |= TRxMatcher(templ);
}

bool TMatchable::operator == (const TMatchable& rhs) const {
    return RxPatterns == rhs.RxPatterns && Equals(SimplePatterns, rhs.SimplePatterns);
}
