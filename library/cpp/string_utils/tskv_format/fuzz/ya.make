FUZZ()

FUZZ_OPTS(
    -max_len=1024
    -rss_limit_mb=8192
)

OWNER(
    dskor
    stakanviski
)

PEERDIR(
    library/cpp/string_utils/tskv_format
)

SRCS(
    test.cpp
)

END()
