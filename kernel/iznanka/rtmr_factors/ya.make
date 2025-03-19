LIBRARY()

OWNER(
    sokursky
)

SRCS(
    indices.cpp
    counters.cpp
)

PEERDIR(
    extsearch/geo/kernel/factor_norm
    kernel/iznanka/rtmr_factors/protos
    library/cpp/string_utils/base64
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(indices.h)

END()
