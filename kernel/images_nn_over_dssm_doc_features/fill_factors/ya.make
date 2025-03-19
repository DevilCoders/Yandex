LIBRARY()

OWNER(
    temnajab
    g:images-followers g:images-robot g:images-search-quality g:images-nonsearch-quality
)

PEERDIR(
    kernel/factor_storage
    kernel/generated_factors_info
    kernel/images_nn_over_dssm_doc_features/factors
    extsearch/images/protos
)

SRCS(
    nn_over_dssm_doc_features.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
