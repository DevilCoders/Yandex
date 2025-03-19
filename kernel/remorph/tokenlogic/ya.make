LIBRARY()

OWNER(g:remorph)

SRCS(
    tokenlogic.cpp
    rule_parser.cpp
    tlmatcher.cpp
    tlresult.cpp
    lexer.l
    parser.y
)

PEERDIR(
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/input
    kernel/remorph/literal
    kernel/remorph/proc_base
    library/cpp/solve_ambig
    library/cpp/containers/sorted_vector
)

END()
