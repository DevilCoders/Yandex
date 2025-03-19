RECURSE_FOR_TESTS(ut)

LIBRARY()

OWNER(
    ptanusha
    g:neural-search
)

SRCS(
    dssm_embeddings.cpp
    embedding_traits.cpp
)

PEERDIR(
    kernel/dssm_applier/begemot
    kernel/dssm_applier/utils
    kernel/nn_ops
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(embedding.h)

END()
