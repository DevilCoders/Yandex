LIBRARY()

OWNER(
    g:base
    mvel
)

PEERDIR(
    library/cpp/packedtypes
    kernel/struct_codegen/metadata
)

SRCS(
    reflection.cpp
)

END()
