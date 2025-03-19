LIBRARY()

OWNER(
    agusakov
    insight
    g:neural-search
)

SRCS(
    apply.cpp
    blob_hash_set.cpp
    compress.cpp
    concat.cpp
    layers.cpp
    optimizations.cpp
    parse.cpp
    remap.cpp
    saveload_utils.h
    states.cpp
    tokenizer.cpp
    updatable_dicts_index.cpp
)


PEERDIR(
    kernel/dssm_applier/nn_applier/lib/optimized_multiply_add
    kernel/dssm_applier/nn_applier/lib/protos
    library/cpp/containers/comptrie
    library/cpp/deprecated/split
    library/cpp/dot_product
    library/cpp/json
    library/cpp/logger/global
    library/cpp/text_processing/dictionary
    library/cpp/threading/local_executor
    library/cpp/containers/flat_hash
    library/cpp/containers/dense_hash
    util
)

GENERATE_ENUM_SERIALIZATION(states.h)
GENERATE_ENUM_SERIALIZATION(updatable_dicts_index.h)

IF (HAVE_MKL)
    PEERDIR(
        contrib/libs/intel/mkl
    )
    SRCS(
        matrix_prod_mkl.cpp
    )
ELSE()
    SRCS(
        matrix_prod.cpp
    )
ENDIF()

END()
