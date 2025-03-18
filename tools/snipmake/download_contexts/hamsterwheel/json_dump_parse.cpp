#include "json_dump_parse.h"

#include <library/cpp/scheme/scheme.h>

#include <util/string/split.h>
#include <util/stream/mem.h>

bool ParseEventlogFromJsonDump(const TString& jsonDump, TRequestBySource& result)
{
    const NSc::TValue dump = NSc::TValue::FromJson(jsonDump);
    if (!dump.IsDict() || !dump.Has("eventlog")) {
        return false;
    }
    TStringBuf eventlog = dump.Get("eventlog").GetString();
    TMemoryInput lines(eventlog);
    TString line;

    THashMap<int, TString> sourceNumbers;

    while (lines.ReadLine(line)) {
        TStringBuf buf(line);
        TVector<TStringBuf> fields;
        StringSplitter(buf).Split('\t').AddTo(&fields);
        if (fields.size() < 8) {
            continue;
        }
        int src = -1;
        if (!TryFromString<int>(fields[3], src) || src < 0) {
            continue;
        }
        if (fields[2] == "SubSourceInit" && fields[7] == "search" && !fields[6].empty()) {
            if (src >= 0) {
                sourceNumbers[src] = TString(fields[6]);
            }
        } else if (fields[2] == "SubSourceRequest") {
            const TString* sourceName = sourceNumbers.FindPtr(src);
            if (!sourceName) {
                continue;
            }
            result[*sourceName] = fields[7];
        }
    }

    return !result.empty();
}
