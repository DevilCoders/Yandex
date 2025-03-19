LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/dssm_applier/nn_applier/lib
    kernel/facts/common
    kernel/facts/dssm_applier
    kernel/snippets/config
    kernel/snippets/factors
    kernel/snippets/qtree
    kernel/snippets/sent_match
)

SRCS(
    calc_dssm.cpp
)

END()
