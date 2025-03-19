#include "string_pool.h"
#include <util/string/join.h>

bool TStringPool::Has(TStringBuf value) const {
    Y_ASSERT(std::is_sorted(Strings.begin(), Strings.end()));
    auto p = std::lower_bound(Strings.begin(), Strings.end(), value);
    if (Y_UNLIKELY(p == Strings.end() || *p != value)) {
        return false;
    }
    return true;
}

TString TStringPool::Get(TStringBuf value) {
    Y_ASSERT(std::is_sorted(Strings.begin(), Strings.end()));
    auto p = std::lower_bound(Strings.begin(), Strings.end(), value);
    if (Y_UNLIKELY(p == Strings.end() || *p != value)) {
        p = Strings.insert(p, ToString(value));
    }
    Y_ASSERT(p != Strings.end());
    return *p;
}

TString TStringPool::GetDebugString() const {
    return JoinSeq(" ", Strings);
}
