LIBRARY()

OWNER(
    g:base
    mvel
    apos
)

PEERDIR(
    kernel/factors_info
    kernel/relevfml
    library/cpp/getopt/small
)

SRCS(
    relev_fml_codegen.cpp
)

END()
