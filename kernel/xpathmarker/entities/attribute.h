#pragma once

#include "attribute_traits.h"

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NHtmlXPath {

struct TAttribute {
    TString Name;
    TString Value;
    EAttributeType Type;
    TPosting Position;

    TAttribute(const TString& name, const TString& value, EAttributeType type = AT_TEXT, TPosting position = 0)
        : Name(name)
        , Value(value)
        , Type(type)
        , Position(position)
    {
    }
};

typedef TVector<TAttribute> TAttributes;

} //namespace NHtmlXPath

