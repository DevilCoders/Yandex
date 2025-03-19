#include "properties.h"

#include <util/generic/singleton.h>
#include <util/generic/map.h>
#include <util/string/vector.h>

namespace NSymbol {

// The names should appear in the same order as in the enumerator
static TStringBuf PROP_NAMES[] = {
    "mw",       // PROP_MULTIWORD
    "cs-lower", // PROP_CASE_LOWER
    "cs-upper", // PROP_CASE_UPPER
    "cs-title", // PROP_CASE_TITLE
    "cs-camel", // PROP_CASE_CAMEL
    "cs-mixed", // PROP_CASE_MIXED
    "cs-1upper",// PROP_CASE_FIRST_UPPER
    "spbf",     // PROP_SPACE_BEFORE
    "alpha",    // PROP_ALPHA
    "num",      // PROP_NUMBER
    "ascii",    // PROP_ASCII
    "nascii",   // PROP_NONASCII
    "nmtoken",  // PROP_NMTOKEN
    "nutoken",  // PROP_NUTOKEN
    "cmp",      // PROP_COMPOUND
    "calpha",   // PROP_CALPHA
    "cnum",     // PROP_CNUMBER
    "first",    // PROP_FIRST
    "last",     // PROP_LAST
    "punct",    // PROP_PUNCT
    "user",     // PROP_USER
};

struct TPropNames : TMap<TStringBuf, EProperty> {
    typedef TMap<TStringBuf, EProperty> TBase;
    TPropNames() {
        for (size_t i = 0; i < Y_ARRAY_SIZE(PROP_NAMES); ++i) {
            TBase::insert(TBase::value_type(PROP_NAMES[i], (EProperty)i));
        }
    }
};

EProperty PropertyByName(const TStringBuf& val) {
    const TPropNames& names = Default<TPropNames>();
    TPropNames::const_iterator i = names.find(val);
    if (names.end() == i) {
        throw yexception() << "Unsupported property name: " << val;
    }
    return i->second;
}


TStringBuf NameByProperty(EProperty prop) {
    size_t i = (size_t)prop;
    if (i < Y_ARRAY_SIZE(PROP_NAMES)) {
        return PROP_NAMES[i];
    }
    return TStringBuf();
}

TPropertyBitSet ParseProperties(const TStringBuf& val) {
    TPropertyBitSet result;
    TVector<TString> props(SplitString(val.data(), ","));
    for (size_t i = 0; i < props.size(); ++i) {
        result.Set(PropertyByName(TStringBuf(props[i].data(), props[i].size())));
    }
    return result;
}

TString ToString(const TPropertyBitSet& props) {
    TString result;
    for (EProperty p : props) {
        if (result)
            result += ",";
        result += NameByProperty(p);
    }
    return result;
}

} // NSymbol
