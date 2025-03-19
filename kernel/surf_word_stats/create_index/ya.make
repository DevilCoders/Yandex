PROGRAM(surf_create_index)

OWNER(smikler)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/getopt
    kernel/surf_word_stats/lib
)

SRCS(
    create_index.cpp
)

END()
