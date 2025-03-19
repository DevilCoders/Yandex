PROGRAM(surf_tool)

OWNER(smikler)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/getopt
    library/cpp/compute_graph
    kernel/surf_word_stats/lib
    ysite/yandex/reqanalysis
)

SRCS(
    tool.cpp
)

END()
