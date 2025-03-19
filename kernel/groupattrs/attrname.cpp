#include "attrname.h"

TString Attrname(const TString& str) {
    size_t dot = str.rfind('.');
    size_t slash = str.rfind('/');
    return TString(str.data(), slash + 1, dot - slash - 1);
}
