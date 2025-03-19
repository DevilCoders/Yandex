OWNER(g:snippets)

LIBRARY()

SRCS(
    answer_models.cpp
    host_stats.cpp
)

PEERDIR(
    kernel/ethos/lib/text_classifier
    kernel/dssm_applier/nn_applier/lib
)

END()
