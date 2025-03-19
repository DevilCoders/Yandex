UNITTEST_FOR(kernel/common_server/util)

OWNER(g:cs_dev)

ALLOCATOR(LF)

FORK_TESTS()

TIMEOUT(180)

SIZE(MEDIUM)

PEERDIR(
    kernel/web_factors_info
    library/cpp/yconf/patcher
    search/idl
)

SRCS(
    events_rate_calcer_ut.cpp
    queue_ut.cpp
    enum_cast_ut.cpp
    util_ut.cpp
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(enum_cast_ut.h)

END()

RECURSE_ROOT_RELATIVE(
    kernel/common_server/util/algorithm/ut
    kernel/common_server/util/types/ut
    kernel/common_server/util/math/ut
    kernel/common_server/util/raw_text/ut
    kernel/common_server/util/object_counter/ut
)
