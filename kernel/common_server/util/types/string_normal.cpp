#include "string_normal.h"

namespace NCS {

    void TStringNormalizer::Trunc(TStringBuf& sb, const char c) {
        while (sb.StartsWith(c)) {
            sb.Skip(1);
        }
        while (sb.EndsWith(c)) {
            sb.Chop(1);
        }
    }

    TString TStringNormalizer::TruncRet(const TStringBuf sb, const char c) {
        TStringBuf sbLocal = sb;
        Trunc(sbLocal, c);
        return TString(sbLocal.data(), sbLocal.size());
    }

    TString TStringNormalizer::TruncRet(const TString& sb, const char c) {
        return TruncRet(TStringBuf(sb.data(), sb.size()), c);
    }

}
