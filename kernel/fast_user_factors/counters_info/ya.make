LIBRARY()

OWNER(
    olegator
    g:search-pers
)

SRCS(
    counters_info.cpp
    counters_names.h
)

PEERDIR(
    kernel/fast_user_factors/counters
    kernel/fast_user_factors/factors_utils
    kernel/fast_user_factors/protos
)

GENERATE_ENUM_SERIALIZATION(counters_names.h)

END()
