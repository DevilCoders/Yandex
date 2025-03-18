LIBRARY()

OWNER(g:passport_infra g:cpp-contrib)

SRCS(
    secret_string.cpp
)

PEERDIR(
    library/cpp/string_utils/ztstrbuf
)

END()

RECURSE_FOR_TESTS(ut)
