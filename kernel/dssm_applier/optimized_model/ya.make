LIBRARY()

OWNER(
    filmih
    g:neural-search
)

SRCS(
    configs_checks.cpp
    format_version.h
    optimized_model_applier.cpp
)

PEERDIR(
    kernel/dssm_applier/begemot
    kernel/dssm_applier/embeddings_transfer
    kernel/dssm_applier/nn_applier/lib
    kernel/dssm_applier/optimized_model/protos
    kernel/dssm_applier/utils
    kernel/embeddings_info
    kernel/factor_storage
    kernel/factors_selector
    kernel/searchlog
    library/cpp/protobuf/yt
    library/cpp/string_utils/base64
    library/cpp/threading/thread_local
    library/cpp/yson/node
)

END()
