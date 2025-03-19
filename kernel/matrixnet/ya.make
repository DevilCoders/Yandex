LIBRARY()

OWNER(
    g:base
    kirillovs
)

SRCS(
    analysis.cpp
    convert.cpp
    dynamic_bundle.cpp
    matrixnet.fbs
    mn_sse.cpp
    mn_sse_model.cpp
    mn_trees.cpp
    mn_dynamic.cpp
    mn_file.cpp
    mn_multi_categ.cpp
    mn_standalone_binarization.cpp
    relev_calcer.cpp
)

PEERDIR(
    contrib/libs/flatbuffers
    kernel/factor_slices
    kernel/factor_storage
    library/cpp/digest/md5
    library/cpp/json
    library/cpp/sse
    library/cpp/threading/thread_local
)

END()
