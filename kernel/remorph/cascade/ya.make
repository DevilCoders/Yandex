LIBRARY()

OWNER(g:remorph)

SRCS(
    cascade.cpp
    cascade_base.cpp
    cascade_item.cpp
    char_item.cpp
    remorph_item.cpp
    tokenlogic_item.cpp
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/common
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/engine/char
    kernel/remorph/input
    kernel/remorph/literal
    kernel/remorph/matcher
    kernel/remorph/proc_base
    kernel/remorph/tokenlogic
    library/cpp/solve_ambig
    library/cpp/containers/sorted_vector
)

END()
