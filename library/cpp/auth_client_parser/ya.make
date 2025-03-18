LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/containers/stack_vector
    library/cpp/digest/old_crc
    library/cpp/string_utils/base64
)

SRCS(
    cookie.cpp
    cookieutils.cpp
    oauth_token.cpp
)

END()

RECURSE_FOR_TESTS(
    examples
    ut
)
