#pragma once

#include <library/cpp/enumbitset/enumbitset.h>

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NRemorph {
namespace NCommon {

enum ERegExprModifier {
    REM_IGNORE_CASE,    // Ignore case.

    REM_MAX
};

typedef TEnumBitSet<ERegExprModifier, REM_IGNORE_CASE, REM_MAX> TRegExprModifiers;

TRegExprModifiers ParseRegExprModifiers(const TString& str);

struct TRegExpr: public TSimpleRefCount<TRegExpr> {
    typedef TIntrusivePtr<TRegExpr> TPtr;

    TRegExMatch Expr;
    TRegExprModifiers Modifiers;

    explicit TRegExpr(const TString& expr, const TRegExprModifiers& modifiers);

    bool Match(const TString& str) const;
    void Save(IOutputStream* out) const;
    static TPtr Load(IInputStream* in);

    bool Match(const TUtf16String& str) const {
        return Match(WideToUTF8(str).data());
    }
};

} // namespace NCommon
} // namespace NRemorph

Y_DECLARE_OUT_SPEC(inline, NRemorph::NCommon::TRegExprModifiers, output, value) {
    if (value.Test(NRemorph::NCommon::REM_IGNORE_CASE)) {
        output << 'i';
    }
}

Y_DECLARE_OUT_SPEC(inline, NRemorph::NCommon::TRegExpr, output, value) {
    output << '/' << value.Expr.GetRegExpr() << '/' << value.Modifiers;
}
