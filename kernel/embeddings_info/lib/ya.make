LIBRARY()

OWNER(
    evgakimutin
    g:neural-search
)

PEERDIR(
    kernel/embeddings_info/proto
    kernel/dssm_applier/utils
    library/cpp/dot_product
)

SRCS(
    tools.cpp
)

END()
