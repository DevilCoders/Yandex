#include "schemaorg_parse.h"

#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/string/strip.h>

namespace NSchemaOrg {
    static const TUtf16String SCHEMA_PREFIX = u"http://schema.org/";
    static const TUtf16String ZERO = u"0";
    static const TUtf16String LEFT_TRASH = u" -*";
    static const TUtf16String LIST_FIELD_DELIMITERS = u",/";
    static const TUtf16String IMDB_BAD_GENRE = u"Genres:";

    TWtringBuf CutSchemaPrefix(TWtringBuf str) {
        if (str.StartsWith(SCHEMA_PREFIX)) {
            return str.substr(SCHEMA_PREFIX.size());
        }
        return str;
    }

    TDuration ParseDuration(TStringBuf str) {
        if (str.EndsWith(" min")) {
            str.Chop(4);
            int minutes = 0;
            TryFromString(str, minutes);
            return TDuration::Minutes(minutes);
        }
        TDuration duration = TDuration::Zero();
        if (str.StartsWith("PT")) {
            str.Skip(2);
            int num = 0;
            for (char c : str) {
                if (IsCommonDigit(c)) {
                    num = num * 10 + (c - '0');
                } else {
                    if (c == 'H') {
                        duration += TDuration::Hours(num);
                    } else if (c == 'M') {
                        duration += TDuration::Minutes(num);
                    } else if (c == 'S') {
                        duration += TDuration::Seconds(num);
                    }
                    num = 0;
                }
            }
        }
        return duration;
    }

    TWtringBuf ParseRating(TWtringBuf rating) {
        while (rating && !IsCommonDigit(rating[0])) {
            rating.Skip(1);
        }
        while (rating && !IsCommonDigit(rating.back())) {
            rating.Chop(1);
        }
        if (rating.Contains('.') || rating.Contains(',')) {
            while (rating && rating.back() == '0') {
                rating.Chop(1);
            }
            if (rating && (rating.back() == '.' || rating.back() == ',')) {
                rating.Chop(1);
            }
        }
        if (rating == ZERO) {
            rating.Clear();
        }
        return rating;
    }

    TWtringBuf CutLeftTrash(TWtringBuf str) {
        size_t pos = str.find_first_not_of(LEFT_TRASH);
        if (pos == str.npos) {
            str.Clear();
        } else if (pos) {
            str.Skip(pos);
        }
        return str;
    }

    TVector<TWtringBuf> ParseList(const TList<TUtf16String>& fields, size_t maxAmount) {
        TVector<TWtringBuf> resultFields;
        for (const TUtf16String& field : fields) {
            TWtringBuf strlist = CutLeftTrash(field);
            while (strlist) {
                size_t pos = strlist.find_first_of(LIST_FIELD_DELIMITERS);
                TWtringBuf tok = StripString(strlist.substr(0, pos));
                strlist = strlist.substr(pos).substr(1);
                if (tok && !IsIn(resultFields, tok)) {
                    resultFields.push_back(tok);
                    if (resultFields.size() == maxAmount) {
                        return resultFields;
                    }
                }
            }
        }
        return resultFields;
    }

    TVector<TWtringBuf> ParseGenreList(const TList<TUtf16String>& fields, size_t maxAmount) {
        // Workaround for imdb.com genre lists like "Drama", "Music", "Genres: Drama | Music"
        if (fields.size() > 0 && fields.size() <= maxAmount && fields.back().StartsWith(IMDB_BAD_GENRE)) {
            maxAmount = fields.size() - 1;
        }
        return ParseList(fields, maxAmount);
    }

} // namespace NSchemaOrg
