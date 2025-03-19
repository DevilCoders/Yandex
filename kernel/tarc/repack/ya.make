LIBRARY()

OWNER(
    bazarinm
    g:base
)

SRCS(
    repack.cpp
)

PEERDIR(
    kernel/tarc/enums
    kernel/tarc/protos
    kernel/tarc/repack/codecs
    library/cpp/codecs
    library/cpp/codecs/static
)

END()
