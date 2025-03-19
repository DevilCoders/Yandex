#pragma once

#include <library/cpp/enumbitset/enumbitset.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSymbol {

enum EProperty {
    PROP_MULTIWORD,         // Input symbol for multiple tokens
    PROP_CASE_LOWER,        // Symbol with lower-case text
    PROP_CASE_UPPER,        // Symbol with upper-case text
    PROP_CASE_TITLE,        // Symbol with first upper-case symbol
    PROP_CASE_CAMEL,        // Symbol with multiple tokens, where at least two ones start from upper-case symbol
    PROP_CASE_MIXED,        // Symbol with mixed-case text
    PROP_CASE_FIRST_UPPER,  // Symbol's first character is upper-case letter.
    PROP_SPACE_BEFORE,      // The symbol has the space before previous symbol in the original source
    PROP_ALPHA,             // Symbol contains only letters.
    PROP_NUMBER,            // Symbol contains only digits.
    PROP_ASCII,             // Symbol contains only ASCII octets.
    PROP_NONASCII,          // Symbol contains only non-ASCII octets.
    PROP_NMTOKEN,           // Symbol is nmtoken.
    PROP_NUTOKEN,           // Symbol is nutoken.
    PROP_COMPOUND,          // Symbol is compound.
    PROP_CALPHA,            // Symbol is compound and its elements contain only letters.
    PROP_CNUMBER,           // Symbol is compound and its elements contain only digits.
    PROP_FIRST,             // Symbol is the first one in the sequence (sentence).
    PROP_LAST,              // Symbol is the last one in the sequence (sentence).
    PROP_PUNCT,             // Symbol is a punctuation mark.
    PROP_USER,              // User-defined property. Can be set for any symbol from the code

    PROP_MAX
};

EProperty PropertyByName(const TStringBuf& val);
TStringBuf NameByProperty(EProperty prop);

typedef TEnumBitSet<EProperty, PROP_MULTIWORD, PROP_MAX> TPropertyBitSet;

TPropertyBitSet ParseProperties(const TStringBuf& val);
TString ToString(const TPropertyBitSet& props);

}  // NSymbol
