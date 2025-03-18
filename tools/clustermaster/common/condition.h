#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

class TCondition;

class TUnaryCondition {
    friend class TCondition;
public:
    enum EKind {
        K_CONSTANT,
        K_EXISTS,
        K_EQUALS,
        K_MAGIC,
    };
private:
    const bool Negative;
    const EKind Kind;
    const TString VariableName;
    const TString Value;

    TUnaryCondition(bool negative, EKind kind, const TString& variableName, const TString& value);
    static TUnaryCondition Parse(const TString&);
public:
    bool IsNegative() const { return Negative; }
    EKind GetKind() const { return Kind; }
    const TString& GetVariableName() const { return VariableName; }
    const TString& GetValue() const { return Value; }
};

class TCondition {
public:
    typedef TVector<TUnaryCondition> TOrArguments;

private:
    const bool Empty;
    const TOrArguments OrArguments;
    const TString Original;

    TCondition(bool isTrue, const TOrArguments& orArguments, const TString& original);

public:
    static TCondition Parse(const TString&);

    bool IsEmpty() const {
        return Empty;
    }

    const TOrArguments& GetOrArguments() const {
        return OrArguments;
    }

    const TString& GetOriginal() const {
        return Original;
    }
};
