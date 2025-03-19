#include "qd_record_type.h"

#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NQueryData {

    const TStringBuf QUERY_KEYWORD{TStringBuf("query")};
    const TStringBuf KEYREF_KEYWORD{TStringBuf("keyref")};
    const TStringBuf COMMON_KEYWORD{TStringBuf("common")};
    const TStringBuf TSTAMP_KEYWORD{TStringBuf("tstamp")};

    const TStringBuf QUERY_DIRECTIVE{TStringBuf("#query")};
    const TStringBuf KEYREF_DIRECTIVE{TStringBuf("#keyref")};
    const TStringBuf COMMON_DIRECTIVE{TStringBuf("#common")};

    const TStringBuf KEYREF_OLD_MR_DIRECTIVE{TStringBuf(":=")};

    TString TRecordType::ToString() const {
        return TStringBuilder() << "#" << Mode << (Timestamp ? TString() + ";" + TSTAMP_KEYWORD + "=" + Timestamp : "");
    }

    TRecordType ParseRecordType(TStringBuf type) {
        TStringBuf dir = type;

        if (KEYREF_OLD_MR_DIRECTIVE == dir) {
            dir = KEYREF_DIRECTIVE;
        }

        TRecordType res;

        if (!dir) {
            res = TRecordType::M_QUERY;
        }

        if (dir.StartsWith('#')) {
            dir.Skip(1);
        }

        while (dir) {
            TStringBuf tok = dir.NextTok(';');

            if (!tok) {
                continue;
            }

            TStringBuf key, val;
            tok.Split('=', key, val);

            if (COMMON_KEYWORD == key) {
                return TRecordType::M_COMMON;
            } else if (KEYREF_KEYWORD == key) {
                return TRecordType::M_KEYREF;
            } else if (QUERY_KEYWORD == key) {
                res.Mode = TRecordType::M_QUERY;
            } else if (TSTAMP_KEYWORD == key) {
                Y_ENSURE(!val || TryFromString<ui64>(val, res.Timestamp),
                       "invalid " << TSTAMP_KEYWORD << " value '" << val << "' in directive '" << type <<"'");
            }
        }

        return res;
    }

}
