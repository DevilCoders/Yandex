UNITTEST()

OWNER(g:remorph)

SRCS(
    input_tree_ut.cpp
    poli_input_ut.cpp
    poli_input_compat_ut.cpp
    re_ut.cpp
)

PEERDIR(
    kernel/remorph/core
    kernel/remorph/core/text
)

END()
