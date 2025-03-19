LIBRARY()

OWNER(g:remorph)

NO_WSHADOW()

SRCS(
    char.cpp
    char_engine.cpp
    regex_token.cpp
    rules_parser.cpp
    regex_lexer.rl6
)

PEERDIR(
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/engine
    kernel/remorph/input
    kernel/remorph/literal
    kernel/remorph/proc_base
    library/cpp/solve_ambig
    library/cpp/unicode/set
)

GENERATE_ENUM_SERIALIZATION(regex_token.h)

END()
