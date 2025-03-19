#pragma once

#include <util/generic/list.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/datetime/base.h>

namespace NSchemaOrg {
    TWtringBuf CutSchemaPrefix(TWtringBuf str);
    TDuration ParseDuration(TStringBuf str);
    TWtringBuf ParseRating(TWtringBuf rating);
    TWtringBuf CutLeftTrash(TWtringBuf str);
    TVector<TWtringBuf> ParseList(const TList<TUtf16String>& fields, size_t maxAmount);
    TVector<TWtringBuf> ParseGenreList(const TList<TUtf16String>& fields, size_t maxAmount);

} // namespace NSchemaOrg
