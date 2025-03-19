#pragma once

// main wares enums: EOntoCatsType, EOntoIntsType
#include <kernel/qtree/richrequest/markup/wtypes/proto/wtypes.pb.h>


#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>


TString ToString(EOntoCatsType wtype);
TString ToString(EOntoIntsType intent);
TString ToString(EOntoCatsType cat, EOntoIntsType intent);

// Specializations for FromString (from util/string/cast.h)
// Note that they are case-insensitive
//template <> EOntoCatsType FromString<>(const TStringBuf& name);
//template <> EOntoIntsType FromString<>(const TStringBuf& name);

struct TSubcat {
    EOntoCatsType Category;
    EOntoIntsType Intent;

    TSubcat()
        : Category(ONTO_UNKNOWN)
        , Intent(ONTO_INTS_UNKNOWN)
    {
    }

    TSubcat(EOntoCatsType cat, EOntoIntsType intent)
        : Category(cat)
        , Intent(intent)
    {
    }

    operator size_t() const {
        return Category * ONTO_CATS_END + Intent;
    }
};




