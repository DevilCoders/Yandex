#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

// required to untie some interfaces
namespace NHtmlDetect {
    class TDetectResult: public THashSet<TString> {
    public:
        void Set(const char* key) {
            (*this).insert(key);
        }
    };

}
