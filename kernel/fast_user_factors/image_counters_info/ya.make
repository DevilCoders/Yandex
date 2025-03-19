LIBRARY()

OWNER(
    g:images-followers g:images-robot g:images-search-quality g:images-nonsearch-quality
)

PEERDIR(
    kernel/fast_user_factors/protos
)

SRCS(image_counters_info.cpp)

GENERATE_ENUM_SERIALIZATION(image_counters_info.h)

END()
