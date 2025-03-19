#pragma once

#include <kernel/yt/dynamic/fwd.h>

class TInstant;

namespace NYT {
    namespace NProtoApi {
        enum class ETimestampAttribute {
            LastCommit /* "last_commit_timestamp" */,
            Retained /* "retained_timestamp" */,
            Unflushed /* "unflushed_timestamp" */
        };

        class TTimestampWithoutTime: public yexception {};

        /**
        * @brief Converts YT dynamic table timestamp to TInstant.
        * @param timestamp YT dynamic table timestamp
        * @return result
        * @throw TTimestampWithoutTime if timestamp is special and doesn't contain time stamp
        */
        TInstant TimeFromYtTimestamp(TTimestamp timestamp);

    }
}
