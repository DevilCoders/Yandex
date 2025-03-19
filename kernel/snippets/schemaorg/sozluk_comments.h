#pragma once

#include <util/generic/list.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSchemaOrg {
    class TSozlukComments {
    public:
        TList<TUtf16String> Comments;

    public:
        TVector<TUtf16String> GetCleanComments(size_t maxCount) const;
        static TUtf16String CleanComment(const TUtf16String& comment);
    };
}
