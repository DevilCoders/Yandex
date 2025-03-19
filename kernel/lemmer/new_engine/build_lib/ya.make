LIBRARY()

OWNER(g:morphology)

SRCS(
    normalize.cpp
    make_schemes.cpp
    make_dict.cpp
    dict_builder.cpp
    common.cpp
)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/lemmer/new_engine/build_lib/protos
    kernel/lemmer/new_engine/binary_dict
    library/cpp/langs
    library/cpp/containers/comptrie
)

END()
