LIBRARY()

OWNER(
    pg
    g:util
)

SRCS(
    md5.cpp
)

PEERDIR(
    contrib/libs/nayuki_md5
    library/cpp/string_utils/base64
)

END()

RECURSE(
    bench
    medium_ut
    ut
)
