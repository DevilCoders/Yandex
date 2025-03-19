LIBRARY()

OWNER(g:remorph)

SRCS(
    proc_base.cpp
    matcher_base.cpp
    result_base.cpp
)

PEERDIR(
    kernel/gazetteer
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/input
    kernel/remorph/literal
    library/cpp/solve_ambig
)

END()
