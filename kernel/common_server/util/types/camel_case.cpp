#include "camel_case.h"
#include <util/charset/wide.h>

namespace NUtil {
    TString ToCamelCase(const TStringBuf sb) {
        TString result;
        bool nextUpper = false;
        for (char c : sb) {
            if (c == '_') {
                nextUpper = true;
            } else if (nextUpper) {
                result += ToUpper(c);
                nextUpper = false;
            } else {
                result += c;
            }
        }
        return result;
    }

    TString ToCamelCase(const TString& s) {
        return ToCamelCase(TStringBuf(s.data(), s.size()));
    }

}
