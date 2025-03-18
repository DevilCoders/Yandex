#pragma once

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <library/cpp/regex/pcre/regexp.h>

#include "compile_regex.h"
#include "str_tools.h"

namespace NCssConfig {
    struct TExprException: public yexception {
    };

    enum EValueType {
        VT_UNKNOWN,
        VT_NUMBER,
        VT_STRING,
        VT_REGEXP,
        VT_URI,
        VT_HASHVALUE,
        VT_FUNCTION,
        VT_EXPRESSION,
        VT_SEPARATOR
    };

    enum EBoolOp {
        BO_AND,
        BO_OR
    };

    struct TContext;

    using TContextList = TVector<TContext>;

    struct TContext {
        TString PropValue;
        double PropValueDouble;
        TString Unit;
        EValueType ValueType;
        TContextList FuncArgs;

        TContext()
            : PropValueDouble(0)
            , ValueType(VT_UNKNOWN)
        {
        }

        TContext(const TContext&) = default;
        TContext& operator=(const TContext&) = default;
    };

    class TExprVal {
    protected:
        EValueType Type;

        union {
            TString* text;
            double fval;
        } Value;
        TRegExMatch* Re; // compiled regular expression from Value.text

    protected:
        void Clear();

    public:
        TExprVal(const TString& text, bool is_regexp);
        TExprVal(double ival);
        TExprVal(const TExprVal& cp);

        virtual ~TExprVal();

        const TExprVal& operator=(const TExprVal& cp);

        virtual EValueType GetType() const;
        virtual double AsDouble(const TContext& context) const;
        virtual TString AsString(const TContext& context) const;
        virtual TRegExMatch* AsRegexp();
        virtual TExprVal* Clone() const;
    };

    class TExprValValue: public TExprVal {
    public:
        TExprValValue()
            : TExprVal("value", false)
        {
        }

        TExprValValue(const TExprValValue& cp)
            : TExprVal(cp)
        {
        }

        double AsDouble(const TContext& context) const override;
        TString AsString(const TContext& context) const override;
        TExprVal* Clone() const override;
    };

    class TExprValUnit: public TExprVal {
    public:
        TExprValUnit()
            : TExprVal("unit", false)
        {
        }

        TExprValUnit(const TExprValUnit& cp)
            : TExprVal(cp)
        {
        }

        double AsDouble(const TContext& context) const override;
        TString AsString(const TContext& context) const override;
        TExprVal* Clone() const override;
    };

    class TBoolExpr {
    public:
        virtual ~TBoolExpr() {
        }
        virtual bool DoEval(const TContext& context) const = 0;
        virtual TBoolExpr* Clone() const = 0;
    };
    /// Base class for binary logical operator (and, or)
    class TBoolExprBinBase: public TBoolExpr {
    protected:
        TBoolExpr* Left;
        TBoolExpr* Right;

    public:
        TBoolExprBinBase(const TBoolExpr& left, const TBoolExpr& right);
        TBoolExprBinBase(const TBoolExprBinBase& copy);

        ~TBoolExprBinBase() override;
    };

    class TBoolExprAnd: public TBoolExprBinBase {
    public:
        TBoolExprAnd(const TBoolExpr& left, const TBoolExpr& right);
        TBoolExprAnd(const TBoolExprAnd& copy);

        bool DoEval(const TContext& context) const override;
        TBoolExpr* Clone() const override;
    };

    class TBoolExprOr: public TBoolExprBinBase {
    public:
        TBoolExprOr(const TBoolExpr& left, const TBoolExpr& right);
        TBoolExprOr(const TBoolExprOr& copy);

        bool DoEval(const TContext& context) const override;
        TBoolExpr* Clone() const override;
    };

    class TBoolExprNot: public TBoolExpr {
        TBoolExpr* Expr;

    public:
        TBoolExprNot(const TBoolExpr& cp);

        ~TBoolExprNot() override;

        bool DoEval(const TContext& context) const override;
        TBoolExpr* Clone() const override;
    };

    class TBoolExprCmpBase: public TBoolExpr {
    protected:
        TExprVal* LeftValue;
        TExprVal* RightValue;

    public:
        TBoolExprCmpBase(const TExprVal& left_value, const TExprVal& right_value);
        TBoolExprCmpBase(const TBoolExprCmpBase& cp);

        ~TBoolExprCmpBase() override;

        TBoolExpr* Clone() const override = 0;
    };

    class TBoolExprCmpLess: public TBoolExprCmpBase {
    public:
        TBoolExprCmpLess(const TExprVal& left_value, const TExprVal& right_value);

        TBoolExprCmpLess(const TBoolExprCmpLess& copy)
            : TBoolExprCmpBase(copy)
        {
        }

        bool DoEval(const TContext& context) const override;

        TBoolExpr* Clone() const override;
    };

    class TBoolExprCmpLessEqual: public TBoolExprCmpBase {
    public:
        TBoolExprCmpLessEqual(const TExprVal& left_value, const TExprVal& right_value);

        TBoolExprCmpLessEqual(const TBoolExprCmpLessEqual& copy)
            : TBoolExprCmpBase(copy)
        {
        }

        bool DoEval(const TContext& context) const override;

        TBoolExpr* Clone() const override;
    };

    class TBoolExprCmpGreat: public TBoolExprCmpBase {
    public:
        TBoolExprCmpGreat(const TExprVal& left_value, const TExprVal& right_value);

        TBoolExprCmpGreat(const TBoolExprCmpGreat& copy)
            : TBoolExprCmpBase(copy)
        {
        }

        bool DoEval(const TContext& context) const override;

        TBoolExpr* Clone() const override;
    };

    class TBoolExprCmpGreatEqual: public TBoolExprCmpBase {
    public:
        TBoolExprCmpGreatEqual(const TExprVal& left_value, const TExprVal& right_value);

        TBoolExprCmpGreatEqual(const TBoolExprCmpGreatEqual& copy)
            : TBoolExprCmpBase(copy)
        {
        }

        bool DoEval(const TContext& context) const override;

        TBoolExpr* Clone() const override;
    };

    class TBoolExprCmpEqual: public TBoolExprCmpBase {
    public:
        TBoolExprCmpEqual(const TExprVal& left_value, const TExprVal& right_value);
        TBoolExprCmpEqual(const TBoolExprCmpEqual& copy)
            : TBoolExprCmpBase(copy)
        {
        }

        bool DoEval(const TContext& context) const override;
        TBoolExpr* Clone() const override;
    };

    class TBoolExprCmpMatch: public TBoolExprCmpBase {
    public:
        TBoolExprCmpMatch(const TExprVal& left_value, const TExprVal& right_value);
        TBoolExprCmpMatch(const TBoolExprCmpMatch& copy)
            : TBoolExprCmpBase(copy)
        {
        }

        bool DoEval(const TContext& context) const override;
        TBoolExpr* Clone() const override;
    };

}
