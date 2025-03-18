#include "condition.h"

#include <library/cpp/deprecated/split/split_iterator.h>

TUnaryCondition TUnaryCondition::Parse(const TString& str) {
    bool negative = str.StartsWith('!');
    TString tail;
    if (negative) {
        tail = str.substr(1);
    } else {
        tail = str;
    }

    if (tail.empty()) {
        return TUnaryCondition(negative, K_CONSTANT, TString(), TString());
    }

    const size_t equalpos = tail.find('=');
    const size_t tildapos = tail.find('~');

    const size_t delimpos = Min(equalpos, tildapos);

    if (delimpos == TString::npos) {
        return TUnaryCondition(negative, K_EXISTS, tail, TString());
    }

    EKind kind = equalpos < tildapos ? K_EQUALS : K_MAGIC;

    TString variableName = tail.substr(0, delimpos);
    TString value = tail.substr(delimpos + 1);

    return TUnaryCondition(negative, kind, variableName, value);
}

TUnaryCondition::TUnaryCondition(bool negative, TUnaryCondition::EKind kind, const TString& variableName, const TString& value)
    : Negative(negative)
    , Kind(kind)
    , VariableName(variableName)
    , Value(value)
{
    if (kind == K_CONSTANT) {
        if (!variableName.empty() || !value.empty()) {
            ythrow yexception() << "params must be undefined when K_CONSTANT";
        }
    } else {
        if (variableName.empty()) {
            ythrow yexception() << "variable name must not be empty when !K_CONSTANT";
        }
    }
}

TCondition::TCondition(bool empty, const TOrArguments& orArguments, const TString& original)
    : Empty(empty)
    , OrArguments(orArguments)
    , Original(original)
{
    if (empty || !orArguments.empty()) {
    } else {
        ythrow yexception() << "invalid combination of parameters";
    }
}

TCondition TCondition::Parse(const TString& str) {
    TVector<TUnaryCondition> r;

    const TSplitDelimiters delims("|");
    const TDelimitersStrictSplit split(str, delims);
    TDelimitersStrictSplit::TIterator it = split.Iterator();

    if (it.Eof()) {
        return TCondition(true, r, str);
    }

    while (!it.Eof())
        r.push_back(TUnaryCondition::Parse(it.NextString()));

    return TCondition(false, r, str);
}

