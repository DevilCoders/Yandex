#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>

namespace NDomSchemeCompiler {

struct TBuiltinType {
    TString SchemeName;
    TString CppName;
    bool CanBeDictKey;
};

struct TBuiltins {
    TVector<TBuiltinType> Types;
    THashMap<TString, const TBuiltinType*> SchemeNameToType;

    TBuiltins() {
        Types.push_back(TBuiltinType{"bool", "bool", false});
        Types.push_back(TBuiltinType{"i8", "i8", true});
        Types.push_back(TBuiltinType{"i16", "i16", true});
        Types.push_back(TBuiltinType{"i32", "i32", true});
        Types.push_back(TBuiltinType{"i64", "i64", true});
        Types.push_back(TBuiltinType{"ui8", "ui8", true});
        Types.push_back(TBuiltinType{"ui16", "ui16", true});
        Types.push_back(TBuiltinType{"ui32", "ui32", true});
        Types.push_back(TBuiltinType{"ui64", "ui64", true});
        Types.push_back(TBuiltinType{"double", "double", false});
        Types.push_back(TBuiltinType{"duration", "TDuration", false});
        Types.push_back(TBuiltinType{"string", "typename TTraits::TStringType", true});
        Types.push_back(TBuiltinType{"any", "__any__", false});

        for (const auto& t : Types) {
            SchemeNameToType[t.SchemeName] = &t;
        }
    }
};

static inline const TBuiltins& Builtins() {
    return *Singleton<TBuiltins>();
};

}
