LIBRARY()

OWNER(
    temnajab
    anskor
    g:images-followers g:images-robot g:images-search-quality g:images-nonsearch-quality
)

SRCS(
    nn_over_dssm_doc_features.cpp
)

PEERDIR(
    kernel/doom/chunked_wad/protos
    kernel/images_nn_over_dssm_doc_features/fill_factors

    extsearch/images/base/factors
    extsearch/images/kernel/knn
    extsearch/images/kernel/nnoptions
    extsearch/images/protos
    extsearch/images/robot/index/protos
)

END()
