PROGRAM()

CFLAGS(-DRFU_ARCADIA_ROOT="${ARCADIA_ROOT}")

OWNER(
    g:base
    mvel
    nkmakarov
)

SRCS(
    relev_fml_unused.cpp
    factors_accountant.cpp
    factors_usage_tester.cpp
)

PEERDIR(
    kernel/matrixnet
    kernel/relevfml
    kernel/relevfml/models_archive
    kernel/web_factors_info
    kernel/generated_factors_info/metadata
    library/cpp/digest/md5
    library/cpp/getopt
    search/web/blender/factors_info
    search/web/blender/factors_info/metadata
)

END()
