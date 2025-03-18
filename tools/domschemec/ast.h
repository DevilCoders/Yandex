#pragma once

#include "builtins.h"

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NDomSchemeCompiler {

struct TStructAttrs {
    enum EPacking {
        DictPacked,
        ArrayPacked,
    };

    // controls validation of fields
    enum EPolicy {
        Flexible, // behavior depends on "strict" argument
        AlwaysStrict, // all fields should by specified by scheme
        AlwaysRelaxed // additional (unspecified) fields are allowed
    };

    EPacking Packing = DictPacked;
    EPolicy Policy = Flexible;
};

struct TFieldAttrs {
    TString CppName;
    TMaybe<size_t> Idx; // makes sense only for ArrayPacked structs. Is always Definied() after parsing for them
    bool Required = false;
    bool Const = false;
};

struct TValidationOperation {
    TString Op; // Op == "code" means value is C++ code,
    TVector<TString> Values; // for everything except Op == "allowed", must contain exectly 1 value
};

struct TValueAttrs {
    TString DefaultValue;
    TVector<TString> DefaultArray;
    TVector<TValidationOperation> ValidationOperators;
};

struct TStruct;

struct TType {
    bool IsTopLevel;

    // actualy is a typed union
    // only one of the following fields must be defined
    TString Name;
    THolder<TStruct> Struct;
    THolder<TType> ArrayOf;
    THolder<std::pair<const TBuiltinType*, TType>> DictOf;

    TType(bool isTopLevel=false)
        : IsTopLevel(isTopLevel)
    {
    }

    TType(const TType& rhs);
    TType& operator=(const TType& rhs);
};

// used for naming typedefs and structs
struct TNamedType {
    TString Name;
    TType Type;

    TNamedType(const TString& name, bool isTopLevel)
        : Name(name)
        , Type(isTopLevel)
    {
    }
};

struct TMemberDefinition
    : public TFieldAttrs
    , public TValueAttrs
{
    TString Name;
    TType Type;
};

struct TParentType {
    TString Name;
    bool IsTopLevel;

    bool operator==(const TParentType& rhs) const {
        return Name == rhs.Name && IsTopLevel == rhs.IsTopLevel;
    }
};

struct TStruct
    : public TStructAttrs
{
    TVector<TParentType> ParentTypes;
    TVector<TNamedType> Types;
    TVector<TMemberDefinition> Members;
    TVector<TValidationOperation> ValidationOperators;
};

inline TType::TType(const TType& rhs) {
    *this = rhs;
}

inline TType& TType::operator=(const TType& rhs) {
    IsTopLevel = rhs.IsTopLevel;
    Name = rhs.Name;
    Struct.Reset(rhs.Struct ? new TStruct(*rhs.Struct) : nullptr);
    ArrayOf.Reset(rhs.ArrayOf ? new TType(*rhs.ArrayOf) : nullptr);
    DictOf.Reset(rhs.DictOf ? new std::pair<const TBuiltinType*, TType>(*rhs.DictOf) : nullptr);
    return *this;
}

struct TNamespace {
    TVector<TString> Name;
    TVector<TNamedType> Types;
};

struct TProgram {
    TVector<TNamespace> Namespaces;
};

};
