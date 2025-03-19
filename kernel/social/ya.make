LIBRARY()

OWNER(
    nkmakarov
    iv-ivan
)

SRCS(
    normalizer.cpp
    recognizer.rl6
)

PEERDIR(
    library/cpp/uri
    kernel/social/protos
    library/cpp/string_utils/url
)

END()
