#include "timestamp.h"

#include <util/datetime/base.h>
namespace NYT {
    namespace NProtoApi {
        namespace {
            bool IsSpecialTimestamp(TTimestamp timestamp) {
                return (timestamp == MinTimestamp ||
                        timestamp == MaxTimestamp ||
                        timestamp == NullTimestamp ||
                        timestamp == SyncLastCommittedTimestamp ||
                        timestamp == AsyncLastCommittedTimestamp ||
                        timestamp == AllCommittedTimestamp);
            }
        }

        TInstant TimeFromYtTimestamp(TTimestamp timestamp) {
            if (Y_UNLIKELY(IsSpecialTimestamp(timestamp)))
                ythrow TTimestampWithoutTime() << "Special TTimestamp doesn't contain time value";
            return TInstant::Seconds(timestamp >> 30 & 0xFFFFFFFF);
        }

    }
}
