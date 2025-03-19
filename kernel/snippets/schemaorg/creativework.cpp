#include "creativework.h"
#include "schemaorg_parse.h"

#include <util/charset/wide.h>
#include <util/string/vector.h>
#include <util/generic/vector.h>

namespace NSchemaOrg {
    TUtf16String TCreativeWork::GetGenres(size_t maxAmount) const {
        TVector<TWtringBuf> genres = ParseGenreList(Genre, maxAmount);
        return JoinStrings(genres.begin(), genres.end(), u", ");
    }

    TUtf16String TCreativeWork::GetAuthors(size_t maxAmount) const {
        TVector<TWtringBuf> authors = ParseList(Author, maxAmount);
        return JoinStrings(authors.begin(), authors.end(), u", ");
    }

} // namespace NSchemaOrg
