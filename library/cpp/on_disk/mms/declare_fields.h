#pragma once

#include <util/system/compiler.h>

namespace NMms {
    template <class TMmsAction>
    void TraverseMany(TMmsAction) {
    }

    template <class TMmsAction, typename T, typename... R>
    void TraverseMany(TMmsAction mmsAction, const T& t, const R&... r) Y_NO_SANITIZE("undefined") {
        TraverseMany(mmsAction(t), r...);
    }
}

#define MMS_DECLARE_FIELDS(...)                                                  \
    template <class TMmsAction>                                                  \
    void traverseFields(TMmsAction mmsAction) const Y_NO_SANITIZE("undefined") { \
        NMms::TraverseMany(mmsAction, __VA_ARGS__);                              \
    }
