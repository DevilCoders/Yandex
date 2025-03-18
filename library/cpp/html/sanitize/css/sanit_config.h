#pragma once

#include <string.h>

#include <util/generic/set.h>
#include <util/generic/yexception.h>
#include <util/generic/string.h>
#include <util/system/compat.h>

#include "str_tools.h"
#include "expressions.h"

namespace NCssConfig {
    using namespace NCssSanit;

    class TEssence {
        TString Text;
        TBoolExpr* Expr;
        bool Regexp;

    public:
        TEssence(const TString& str, bool regexp, const TBoolExpr* expr = nullptr);
        TEssence(const TEssence& copy);
        ~TEssence();

        TEssence& operator=(const TEssence& copy);

        const TString& GetText() const {
            return Text;
        }
        bool IsRegexp() const {
            return Regexp;
        }
        const TBoolExpr* GetExpr() const {
            return Expr;
        }

        void AssignExpr(const TBoolExpr* expr);
    };

    namespace NPrivate {
        struct LessCI {
            bool operator()(const TEssence& t1, const TEssence& t2) const {
                return stricmp(t1.GetText().c_str(), t2.GetText().c_str()) < 0 ? true : false;
            }
        };
    } //namespace NPrivate

    template <class T>
    class TEmptible {
        T Value;
        bool Empty;

    public:
        typedef T ValueType;

        TEmptible()
            : Value(ValueType())
            , Empty(true)
        {
        }

        TEmptible(const ValueType& value)
            : Value(value)
            , Empty(false)
        {
        }

        TEmptible(const TEmptible&) = default;

        TEmptible& operator=(const ValueType& value) {
            Empty = false;
            Value = value;
            return *this;
        }

        TEmptible& operator=(const TEmptible&) = default;

        operator const ValueType&() const {
            return Value;
        }

        ValueType& Get() {
            return Value;
        }
        ValueType* operator->() {
            return &Value;
        }

        const ValueType* operator->() const {
            return &Value;
        }

        bool IsEmpty() const {
            return Empty;
        }

        void SetEmpty() {
            Empty = true;
        }
    };

    class TEssenceSet {
    public:
        typedef TSet<TEssence, NPrivate::LessCI> TEssenceSetType;

    private:
        TEssenceSetType Set;

    public:
        TEssenceSet() {
        }
        TEssenceSet(const TEssenceSet& cp);

        ~TEssenceSet();

        TEssenceSet& operator=(const TEssenceSet& cp);

        void Add(const TString& name, bool is_regexp = false);

        /// Insert essence into the set and take ownerhip (will free pointer on destroying)
        void Add(const TEssence& essence);

        void AddEssenceSet(const TEssenceSet& set);
        void Clear();
        const TEssenceSetType& GetEssences() const;
    };

    enum EActionType {
        AT_EMPTY,
        AT_DENY,
        AT_PASS,
        AT_MERGE,
        AT_REWRITE
    };

    struct TImportAction {
        EActionType Action;
        TString StrParam;

        TImportAction();
        TImportAction(EActionType action, const TString& param);
        void Clear();
    };

    class TConfig {
    public:
        struct Error: public yexception {
            Error(const TString& str) {
                *this << str;
            }
        };

    public:
        TConfig();

        void InitDefaults();

        TEmptible<bool> DefaultPass;
        TEmptible<bool> ExpressionPass;
        TEmptible<TImportAction> ImportAction;

        TEssenceSet& PropertyPass() {
            return PropSetPass;
        }
        const TEssenceSet& PropertyPass() const {
            return PropSetPass;
        }

        TEssenceSet& PropertyDeny() {
            return PropSetDeny;
        }
        const TEssenceSet& PropertyDeny() const {
            return PropSetDeny;
        }

        TEssenceSet& SelectorPass() {
            return SelectorSetPass;
        }
        const TEssenceSet& SelectorPass() const {
            return SelectorSetPass;
        }

        TEssenceSet& SelectorDeny() {
            return SelectorSetDeny;
        }
        const TEssenceSet& SelectorDeny() const {
            return SelectorSetDeny;
        }

        TEssenceSet& SchemePass() {
            return SchemeSetPass;
        }
        const TEssenceSet& SchemePass() const {
            return SchemeSetPass;
        }

        TEssenceSet& SchemeDeny() {
            return SchemeSetDeny;
        }
        const TEssenceSet& SchemeDeny() const {
            return SchemeSetDeny;
        }

        TEssenceSet& ClassDeny() {
            return ClassSetDeny;
        }
        const TEssenceSet& ClassDeny() const {
            return ClassSetDeny;
        }

        TEssenceSet& ClassPass() {
            return ClassSetPass;
        }
        const TEssenceSet& ClassPass() const {
            return ClassSetPass;
        }

        void AddSelectorAppend(const TStrokaList& list);
        const TStrokaList& SelectorAppend() const {
            return SelectorAppendList;
        }

    private:
        TEssenceSet PropSetPass;
        TEssenceSet PropSetDeny;

        TEssenceSet SelectorSetPass;
        TEssenceSet SelectorSetDeny;

        TEssenceSet SchemeSetPass;
        TEssenceSet SchemeSetDeny;

        TEssenceSet ClassSetPass;
        TEssenceSet ClassSetDeny;

        TStrokaList SelectorAppendList;
    };

    inline TEssence::TEssence(const TString& str, bool regexp, const TBoolExpr* expr)
        : Text(str)
        , Expr(expr ? expr->Clone() : nullptr)
        , Regexp(regexp)
    {
    }

    inline TEssence::TEssence(const TEssence& copy)
        : Text(copy.Text)
        , Expr(copy.Expr ? copy.Expr->Clone() : nullptr)
        , Regexp(copy.Regexp)
    {
    }

    inline TEssence::~TEssence() {
        delete Expr;
    }

    inline void TEssence::AssignExpr(const TBoolExpr* expr) {
        delete this->Expr;

        if (expr)
            this->Expr = expr->Clone();
        else
            this->Expr = nullptr;
    }

    inline TConfig::TConfig() {
        InitDefaults();
    }

    inline TEssenceSet::TEssenceSet(const TEssenceSet& cp)
        : Set(cp.Set)
    {
    }

    inline TEssenceSet::~TEssenceSet() {
        Clear();
    }

    inline TEssenceSet& TEssenceSet::operator=(const TEssenceSet& cp) {
        Set = cp.Set;
        return *this;
    }

    inline void TEssenceSet::Clear() {
        Set.clear();
    }

    inline void TEssenceSet::Add(const TString& prop_name, bool is_regexp) {
        Set.insert(TEssence(prop_name, is_regexp));
    }

    inline void TEssenceSet::Add(const TEssence& essence) {
        Set.insert(essence);
    }

    inline const TEssenceSet::TEssenceSetType& TEssenceSet::GetEssences() const {
        return Set;
    }

    inline TImportAction::TImportAction()
        : Action(AT_EMPTY)
    {
    }

    inline TImportAction::TImportAction(EActionType action, const TString& param)
        : Action(action)
        , StrParam(param)
    {
    }

    inline void TImportAction::Clear() {
        Action = AT_EMPTY;
        StrParam.clear();
    }

}
