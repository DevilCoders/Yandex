LIBRARY()

OWNER(g:remorph)

SRCS(
    matcher.cpp
    matcher.h
    remorph_matcher.cpp
    remorph_matcher.h
    rule_lexer_.cpp
    rule_parser.cpp
    rule_parser.h
    rule_lexer.l
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/common
    kernel/lemmer/dictlib
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/input
    kernel/remorph/literal
    kernel/remorph/proc_base
    library/cpp/solve_ambig
    library/cpp/token
    util/draft
)

END()
