LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/string_utils/base64
)

SRCS(
    interval.cpp
    string_pool.cpp
    cache_with_age.cpp
    camel_case.cpp
    string_normal.cpp
)

END()

RECURSE_FOR_TESTS(ut)
