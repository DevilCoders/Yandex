#include "reg_expr.h"

#include <util/ysaveload.h>
#include <util/generic/yexception.h>

#include <contrib/libs/pcre/pcreposix.h>

namespace NRemorph {
namespace NCommon {

namespace {

inline int ToFlags(const TRegExprModifiers& regExprModifiers) {
    int flags = REG_EXTENDED | REG_UTF8;
    if (regExprModifiers.Test(REM_IGNORE_CASE)) {
        flags |= REG_ICASE;
    }
    return flags;
}

}

TRegExprModifiers ParseRegExprModifiers(const TString& str) {
    TRegExprModifiers regExprModifiers;
    for (const char* c = str.begin(); c != str.end(); ++c) {
        switch (*c) {
            case 'i':
                regExprModifiers.Set(REM_IGNORE_CASE);
                break;
            default:
                // TODO: Customize exceptions.
                throw yexception() << "Unknown regular expression modifier: " << *c;
        }
    }
    return regExprModifiers;
}

TRegExpr::TRegExpr(const TString& expr, const TRegExprModifiers& modifiers)
    : Expr(expr.data(), ToFlags(modifiers))
    , Modifiers(modifiers)
{
}

bool TRegExpr::Match(const TString& str) const {
    return Expr.Match(str.data());
}

void TRegExpr::Save(IOutputStream* out) const {
    ::Save(out, Expr.GetRegExpr());
    ::Save(out, Modifiers);
}

TRegExpr::TPtr TRegExpr::Load(IInputStream* in) {
    TString expr;
    TRegExprModifiers modifiers;
    ::Load(in, expr);
    ::Load(in, modifiers);
    return new TRegExpr(expr, modifiers);
}

} // namespace NCommon
} // namespace NRemorph
