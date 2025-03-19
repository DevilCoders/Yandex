LIBRARY()

OWNER(g:remorph)

SRCS(
    engine.cpp
    parse_options.cpp
    parse_token.cpp
    pre_parser.cpp
    lexer.rl6
)

PEERDIR(
    kernel/remorph/common
)

GENERATE_ENUM_SERIALIZATION(parse_token.h)

END()

