LIBRARY()

OWNER(
    agusakov
    g:base
    g:neural-search
)

SRCS(
    dssm_embedding.h
)

PEERDIR(
    kernel/dssm_applier/begemot
    kernel/dssm_applier/decompression
    kernel/dssm_applier/utils
    kernel/dssm_applier/nn_applier/lib
    kernel/dssm_applier/optimized_model
    kernel/dssm_applier/query_word_title
    kernel/embeddings_info
    kernel/factor_storage
    library/cpp/dot_product
    library/cpp/string_utils/base64
)

END()

RECURSE(
    pylib
)
