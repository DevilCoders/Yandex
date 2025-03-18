#include "custom_options.h"

#include <util/string/cast.h>

template <>
TCustomOptions FromStringImpl<TCustomOptions>(const char* b, size_t n) {
    TStringBuf config(b, n);
    TStringBuf v1, v2;
    config.Split(',', v1, v2);
    TCustomOptions value;
    value.V1 = TString(v1);
    value.V2 = TString(v2);
    return value;
}
