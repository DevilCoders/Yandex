#include "expressions.h"

namespace NCssConfig {
    TExprVal::TExprVal(const TString& text, bool is_regexp)
        : Type(is_regexp ? VT_REGEXP : VT_STRING)
        , Re(nullptr)
    {
        Value.text = new TString(text);

        if (is_regexp)
            Re = CompileRegexp(text);
    }

    TExprVal::TExprVal(double val)
        : Type(VT_NUMBER)
        , Re(nullptr)
    {
        Value.fval = val;
    }

    TExprVal::TExprVal(const TExprVal& cp)
        : Type(VT_UNKNOWN)
    {
        operator=(cp);
    }

    void TExprVal::Clear() {
        switch (Type) {
            case VT_REGEXP:
                delete Re;
                Re = nullptr;
                [[fallthrough]]; // Go next, the string is also must be freed
            case VT_STRING:
                delete Value.text;
                Value.text = nullptr;
                [[fallthrough]];
            default:
                break;
        }
        Type = VT_UNKNOWN;
    }

    const TExprVal& TExprVal::operator=(const TExprVal& cp) {
        Clear();

        switch (cp.Type) {
            case VT_STRING:
                Value.text = new TString(*cp.Value.text);
                break;

            case VT_REGEXP:
                Value.text = new TString(*cp.Value.text);
                Re = CompileRegexp(*Value.text);
                break;

            case VT_NUMBER:
                Value.fval = cp.Value.fval;
                break;

            default:
                ythrow TExprException() << "Unknown value type.";
        }
        Type = cp.Type;
        return *this;
    }

    TExprVal::~TExprVal() {
        switch (Type) {
            case VT_REGEXP:
                delete Re;
                [[fallthrough]];
            case VT_STRING:
                delete Value.text;
                [[fallthrough]];
            default:
                break;
        }
    }

    EValueType TExprVal::GetType() const {
        return Type;
    }

    double TExprVal::AsDouble(const TContext&) const {
        if (Type == VT_NUMBER)
            return Value.fval;
        else
            ythrow TExprException() << "Can't convert to double";
    }

    TString TExprVal::AsString(const TContext&) const {
        if (Type == VT_STRING)
            return *Value.text;
        else
            ythrow TExprException() << "Can't convert to string";
    }

    TRegExMatch* TExprVal::AsRegexp() {
        if (Re)
            return Re;
        else
            ythrow TExprException() << "Value is not regular expression";
    }
    ///////////
    TExprVal* TExprVal::Clone() const {
        return new TExprVal(*this);
    }

    double TExprValValue::AsDouble(const TContext& context) const {
        return context.PropValueDouble;
    }

    TString TExprValValue::AsString(const TContext& context) const {
        return context.PropValue;
    }
    ///////////
    TExprVal* TExprValValue::Clone() const {
        return new TExprValValue(*this);
    }

    double TExprValUnit::AsDouble(const TContext&) const {
        ythrow TExprException() << "Can't convert to double";
    }

    TString TExprValUnit::AsString(const TContext& context) const {
        return context.Unit;
    }

    TExprVal* TExprValUnit::Clone() const {
        return new TExprValUnit(*this);
    }

    TBoolExprBinBase::TBoolExprBinBase(const TBoolExpr& left, const TBoolExpr& right)
        : Left(left.Clone())
        , Right(right.Clone())
    {
    }

    TBoolExprBinBase::TBoolExprBinBase(const TBoolExprBinBase& copy)
        : TBoolExpr()
        , Left(copy.Left->Clone())
        , Right(copy.Right->Clone())
    {
    }

    TBoolExprBinBase::~TBoolExprBinBase() {
        delete Left;
        delete Right;
    }

    /////////////////

    TBoolExprAnd::TBoolExprAnd(const TBoolExpr& left, const TBoolExpr& right)
        : TBoolExprBinBase(left, right)
    {
    }

    TBoolExprAnd::TBoolExprAnd(const TBoolExprAnd& copy)
        : TBoolExprBinBase(copy)
    {
    }

    bool TBoolExprAnd::DoEval(const TContext& context) const {
        if (!Left->DoEval(context))
            return false;

        if (!Right->DoEval(context))
            return false;

        return true;
    }

    TBoolExpr* TBoolExprAnd::Clone() const {
        return new TBoolExprAnd(*this);
    }
    ///////////////

    TBoolExprOr::TBoolExprOr(const TBoolExpr& left, const TBoolExpr& right)
        : TBoolExprBinBase(left, right)
    {
    }

    TBoolExprOr::TBoolExprOr(const TBoolExprOr& copy)
        : TBoolExprBinBase(copy)
    {
    }

    bool TBoolExprOr::DoEval(const TContext& context) const {
        if (Left->DoEval(context))
            return true;

        if (Right->DoEval(context))
            return true;

        return false;
    }

    TBoolExpr* TBoolExprOr::Clone() const {
        return new TBoolExprOr(*this);
    }
    /////////////

    TBoolExprNot::TBoolExprNot(const TBoolExpr& cp)
        : Expr(cp.Clone())
    {
    }

    TBoolExprNot::~TBoolExprNot() {
        delete Expr;
    }

    bool TBoolExprNot::DoEval(const TContext& context) const {
        return !Expr->DoEval(context);
    }

    TBoolExpr* TBoolExprNot::Clone() const {
        return new TBoolExprNot(*this);
    }
    //////////////

    TBoolExprCmpBase::TBoolExprCmpBase(const TExprVal& left_value, const TExprVal& right_value)
        : LeftValue(left_value.Clone())
        , RightValue(right_value.Clone())
    {
    }

    TBoolExprCmpBase::TBoolExprCmpBase(const TBoolExprCmpBase& cp)
        : TBoolExpr()
        , LeftValue(cp.LeftValue->Clone())
        , RightValue(cp.RightValue->Clone())
    {
    }

    TBoolExprCmpBase::~TBoolExprCmpBase() {
        delete LeftValue;
        delete RightValue;
    }

    //////////////////

    TBoolExprCmpLess::TBoolExprCmpLess(const TExprVal& left_value, const TExprVal& right_value)
        : TBoolExprCmpBase(left_value, right_value)
    {
    }

    bool TBoolExprCmpLess::DoEval(const TContext& context) const {
        double lval = LeftValue->AsDouble(context);
        double rval = RightValue->AsDouble(context);

        return lval < rval;
    }

    TBoolExpr* TBoolExprCmpLess::Clone() const {
        return new TBoolExprCmpLess(*this);
    }
    //////////////

    TBoolExprCmpLessEqual::TBoolExprCmpLessEqual(const TExprVal& left_value, const TExprVal& right_value)
        : TBoolExprCmpBase(left_value, right_value)
    {
    }

    bool TBoolExprCmpLessEqual::DoEval(const TContext& context) const {
        double lval = LeftValue->AsDouble(context);
        double rval = RightValue->AsDouble(context);

        return lval <= rval;
    }

    TBoolExpr* TBoolExprCmpLessEqual::Clone() const {
        return new TBoolExprCmpLessEqual(*this);
    }
    //////////////

    TBoolExprCmpGreat::TBoolExprCmpGreat(const TExprVal& left_value, const TExprVal& right_value)
        : TBoolExprCmpBase(left_value, right_value)
    {
    }

    bool TBoolExprCmpGreat::DoEval(const TContext& context) const {
        double lval = LeftValue->AsDouble(context);
        double rval = RightValue->AsDouble(context);

        return lval > rval;
    }

    TBoolExpr* TBoolExprCmpGreat::Clone() const {
        return new TBoolExprCmpGreat(*this);
    }

    /////////////////

    TBoolExprCmpGreatEqual::TBoolExprCmpGreatEqual(const TExprVal& left_value, const TExprVal& right_value)
        : TBoolExprCmpBase(left_value, right_value)
    {
    }

    bool TBoolExprCmpGreatEqual::DoEval(const TContext& context) const {
        double lval = LeftValue->AsDouble(context);
        double rval = RightValue->AsDouble(context);

        return lval >= rval;
    }

    TBoolExpr* TBoolExprCmpGreatEqual::Clone() const {
        return new TBoolExprCmpGreatEqual(*this);
    }

    //////////////////

    TBoolExprCmpEqual::TBoolExprCmpEqual(const TExprVal& left_value, const TExprVal& right_value)
        : TBoolExprCmpBase(left_value, right_value)
    {
    }

    bool TBoolExprCmpEqual::DoEval(const TContext& context) const {
        double lval = LeftValue->AsDouble(context);
        double rval = RightValue->AsDouble(context);

        return lval == rval;
    }

    TBoolExpr* TBoolExprCmpEqual::Clone() const {
        return new TBoolExprCmpEqual(*this);
    }

    //////////////

    TBoolExprCmpMatch::TBoolExprCmpMatch(const TExprVal& left_value, const TExprVal& right_value)
        : TBoolExprCmpBase(left_value, right_value)
    {
    }

    bool TBoolExprCmpMatch::DoEval(const TContext& context) const {
        TString lval = LeftValue->AsString(context);
        TRegExMatch* re = RightValue->AsRegexp();

        return re->Match(lval.c_str());
    }

    TBoolExpr* TBoolExprCmpMatch::Clone() const {
        return new TBoolExprCmpMatch(*this);
    }

}
