#pragma once

#include <util/generic/maybe.h>
#include <util/string/cast.h>

template <typename T>
TString ToString(const TMaybe<T>& what) {
    if (what.Defined()) {
        return TString("TMaybe(").append(ToString(*what)).append(')');
    } else {
        return TString("TMaybe()");
    }
}

