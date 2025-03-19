LIBRARY()

OWNER(
    g:base
    mvel
)

SRCS(
    xmap.cpp
    xref_types.cpp
)

PEERDIR(
    kernel/lemmer/alpha
    kernel/mango/proto
    kernel/struct_codegen/reflection
    kernel/xref/enums
    library/cpp/on_disk/2d_array
    library/cpp/on_disk/4d_array
    library/cpp/on_disk/head_ar
    library/cpp/packedtypes
)

STRUCT_CODEGEN(xmap_gen)

GENERATE_ENUM_SERIALIZATION(xmap.h)

END()
