LIBRARY()

OWNER(
    g:util
    g:middle
    rumvadim
    and42
    petrk
)

SRCS(
    http_parser.cpp
)

PEERDIR(
    library/cpp/http/io
    library/cpp/blockcodecs
)

END()

RECURSE_FOR_TESTS(ut)
