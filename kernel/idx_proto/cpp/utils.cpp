#include "utils.h"
#include <util/string/cast.h>

TMaybe<ui64> TryParseShardTimestamp(const TStringBuf shardValue) {
    TStringBuf timestampCandidate(shardValue);
    timestampCandidate = timestampCandidate.RNextTok('-');
    ui64 timestamp;
    return (TryFromString<ui64>(timestampCandidate, timestamp)) ? MakeMaybe(timestamp) : Nothing();
}
