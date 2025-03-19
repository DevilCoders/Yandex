LIBRARY()

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/packedtypes
    kernel/struct_codegen/metadata
    kernel/struct_codegen/reflection
    kernel/xref
    ysite/yandex/erf
)

SRCS(
    struct_print.cpp
)

END()
