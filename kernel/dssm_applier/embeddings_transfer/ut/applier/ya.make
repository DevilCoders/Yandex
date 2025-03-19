PROGRAM()

OWNER(
    filmih
    g:neural-search
    e-shalnov
)


SRCS(
    main.cpp
)

PEERDIR (
    kernel/dssm_applier/embeddings_transfer
    kernel/dssm_applier/embeddings_transfer/ut/applier/protos
    library/cpp/getoptpb
    library/cpp/getoptpb/proto
    library/cpp/threading/future
)

END()
