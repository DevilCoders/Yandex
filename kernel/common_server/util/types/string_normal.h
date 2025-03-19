#pragma once
#include <util/generic/string.h>

namespace NCS {
    class TStringNormalizer {
    public:
        static void Trunc(TStringBuf& sb, const char c);
        static TString TruncRet(const TStringBuf sb, const char c);
        static TString TruncRet(const TString& sb, const char c);
    };
}
