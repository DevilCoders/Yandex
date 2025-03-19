LIBRARY()

OWNER(
    filmih
    g:neural-search
)

SRCS(
    embeddings_transfer.cpp
)

PEERDIR(
    kernel/dssm_applier/embeddings_transfer/proto
    kernel/dssm_applier/utils
)

END()
