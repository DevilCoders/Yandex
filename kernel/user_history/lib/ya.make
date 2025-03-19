LIBRARY()

OWNER(
    g:search-pers
)

SRCS(
    compression_tools.cpp
    descriptions_to_log.cpp
    embedding_tools.cpp
    fading_embedding_tools.cpp
    feature_calcers.cpp
    merge.cpp
)

PEERDIR(
    kernel/dssm_applier/begemot
    kernel/dssm_applier/decompression
    kernel/dssm_applier/utils
    kernel/embeddings_info/lib
    kernel/vector_machine
    kernel/user_history
    kernel/user_history/proto
    library/cpp/dot_product
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    yabs/proto
    ysite/yandex/reqanalysis
)

BASE_CODEGEN(kernel/vector_machine/codegen calcer_codegen)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(calcer_codegen.h)
GENERATE_ENUM_SERIALIZATION(merge.h)

END()
