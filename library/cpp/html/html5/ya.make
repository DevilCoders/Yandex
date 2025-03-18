LIBRARY()

OWNER(stanly)

SRCS(
    tag.gperf
    foreign.cpp
    lexer.cpp
    output.cpp
    parse.cpp
    parser.cpp
    text_normalize.cpp
    twitter.cpp
)

PEERDIR(
    contrib/libs/libc_compat
    library/cpp/html/face
    library/cpp/html/spec
)

GENERATE_ENUM_SERIALIZATION(error.h)

END()
