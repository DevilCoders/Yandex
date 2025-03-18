#pragma once

#include "condition.h"
#include "printer.h"

#include <library/cpp/deprecated/split/split_iterator.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/ascii.h>

struct TVariablesMessage;
struct TConfigMessage;

namespace NPrivate {

struct TVariable {
    typedef TString TName;
    typedef TString TValue;

    TName Name;
    TValue Value;

    TVariable() {
    }

    TVariable(const TName& name, const TValue& value)
        : Name(name)
        , Value(value)
    {
    }

    struct TExtractName {
        template <class T>
        const TName& operator()(const T& from) const noexcept {
            return from.Name;
        }
    };
};

struct TVariableWithRevision: TVariable {
    typedef size_t TRevision;

    TRevision Revision;

    TVariableWithRevision()
        : Revision(0)
    {
    }

    TVariableWithRevision(const TVariable::TName& name, const TVariable::TValue& value, TRevision revision)
        : TVariable(name, value)
        , Revision(revision)
    {
    }
};

template <class T>
struct TVariablesMap
    : THashTable<T, typename T::TName, THash<typename T::TName>, typename T::TExtractName, TEqualTo<typename T::TName>, std::allocator<T>>
{
    typedef THashTable<T, typename T::TName, THash<typename T::TName>, typename T::TExtractName, TEqualTo<typename T::TName>, std::allocator<T>> TBase;
    typedef T TVar;

    TVariablesMap()
        : TBase(7, typename TBase::hasher(), typename TBase::key_equal())
    {
    }
};

template <class T, class PVar>
struct TWorkerVariablesBase {
    typedef PVar TVar;

    typedef TWorkerVariablesBase<T, TVar> TBase;

    typedef TVariablesMap<TVar> TMapType;

    inline void Clear();

    inline bool Has(const typename TVar::TName& name) const;
    inline bool Get(TVar& variable) const;
    inline bool Set(const TVar& variable);
    inline bool Del(const typename TVar::TName& name);

    bool CheckUnaryCondition(const TUnaryCondition& condition) const;
    bool CheckCondition(const TCondition& condition) const;

    void DumpState(TPrinter&) const;

protected:
    TMapType Map;
};

} // NPrivate

class TWorkerVariablesLight: public NPrivate::TWorkerVariablesBase<TWorkerVariablesLight, NPrivate::TVariable> {
public:
    bool Update(const TVariablesMessage& message);

    const TMapType& GetMap() const noexcept {
        return Map;
    }

    friend struct NPrivate::TWorkerVariablesBase<TWorkerVariablesLight, TMapType>;
};

class TWorkerVariables: public NPrivate::TWorkerVariablesBase<TWorkerVariables, NPrivate::TVariableWithRevision> {
public:
    static constexpr auto VariablesYtKey = "#variables";

public:
    struct TNoVariable: yexception {};

    TWorkerVariables()
        : LastRevision(1)
    {
    }

    NPrivate::TVariableWithRevision::TRevision GetLastRevision() const noexcept {
        return LastRevision;
    }

    NPrivate::TVariableWithRevision::TRevision IncLastRevision() noexcept {
        return ++LastRevision;
    }

    const TMapType& GetMap() const noexcept {
        return Map;
    }

    bool Update(const TVariablesMessage& message);

    void ApplyStrongVariable(const TString& name, const TString& value);
    void ApplyDefaultVariable(const TString& name, const TString& value);

    void SerializeToArcadiaStream(IOutputStream* out) const;
    void ParseFromArcadiaStream(IInputStream* in);

    NPrivate::TVariableWithRevision::TRevision Substitute(TString& s) const;

    friend struct NPrivate::TWorkerVariablesBase<TWorkerVariables, TMapType>;

private:
    NPrivate::TVariableWithRevision::TRevision LastRevision;
};

namespace NPrivate {

template <class T, class M>
inline void TWorkerVariablesBase<T, M>::Clear() {
    Map.clear();
}

template <class T, class M>
inline bool TWorkerVariablesBase<T, M>::Has(const typename TVar::TName& name) const {
    return Map.find(name) != Map.end();
}

template <class T, class M>
inline bool TWorkerVariablesBase<T, M>::Get(TVar& variable) const {
    const typename TMapType::const_iterator ret = Map.find(variable.Name);

    if (ret == Map.end())
        return false;

    variable = *ret;

    return true;
}

template <class T, class M>
inline bool TWorkerVariablesBase<T, M>::Set(const TVar& variable) {
    const std::pair<typename TMapType::iterator, bool> ret = Map.insert_unique(variable);

    if (ret.second)
        return true;
    if (ret.first->Value == variable.Value)
        return false;

    *ret.first = variable;

    return true;
}

template <class T, class M>
inline bool TWorkerVariablesBase<T, M>::Del(const typename TVar::TName& name) {
    return Map.erase(name);
}

template <class T, class M>
bool TWorkerVariablesBase<T, M>::CheckUnaryCondition(const TUnaryCondition &condition) const {
    // TODO: Make it good

    const T* const This = static_cast<const T*>(this);

    if (condition.GetKind() == TUnaryCondition::K_CONSTANT)
        return !condition.IsNegative();

    if (condition.GetKind() == TUnaryCondition::K_EXISTS)
        return This->Has(condition.GetVariableName()) != condition.IsNegative();

    TVar variable;

    variable.Name = condition.GetVariableName();

    if (variable.Name.empty() || !This->Get(variable))
        return condition.IsNegative();

    TString first(variable.Value);
    TString second = condition.GetValue();

    if (!second.empty() && second[0] == '$') {
        TString(second, 1, TString::npos).swap(variable.Name);

        if (!This->Get(variable))
            return condition.IsNegative();

        second.swap(variable.Value);
    }

    if (condition.GetKind() == TUnaryCondition::K_EQUALS) {  // Value of a variable
        return (first == second) != condition.IsNegative();
    } else if (condition.GetKind() == TUnaryCondition::K_MAGIC) {  // Contents of a variable
        if (second.empty())
            return !condition.IsNegative();

        for (size_t pos = first.find(second); pos != TString::npos; pos = first.find(second, pos + 1)) {
            if (pos > 0 && !IsAsciiSpace(first[pos - 1]))
                continue; // value found, but it's part of larger word (to the left)

            if (pos + second.length() < first.length() && !IsAsciiSpace(first[pos + second.length()]))
                continue; // value found, but it's part of larger word (to the right)

            return !condition.IsNegative();
        }

        return condition.IsNegative();
    }

    Y_FAIL("How does control reach this line?");
}

template <class T, class M>
bool TWorkerVariablesBase<T, M>::CheckCondition(const TCondition& condition) const {
    const T* const This = static_cast<const T*>(this);

    if (condition.IsEmpty())
        return true;

    for (TCondition::TOrArguments::const_iterator orArg = condition.GetOrArguments().begin(); orArg != condition.GetOrArguments().end(); ++orArg) {
        if (This->CheckUnaryCondition(*orArg))
            return true;
    }

    return false;
}

template <class T, class M>
void TWorkerVariablesBase<T, M>::DumpState(TPrinter& out) const {
    out.Println("Variables:");
    TPrinter l1 = out.Next();
    for (typename TMapType::const_iterator entry = Map.begin(); entry != Map.end(); ++entry) {
        l1.Println(ToString(*entry));
    }
    out.Println("Variables.");
}

} // NPrivate

size_t SubstituteNumericArg(TString& s, size_t n, const TString& to);
