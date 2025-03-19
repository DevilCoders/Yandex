LIBRARY()

OWNER(g:morphology)

SRCS(
    new_generator.cpp
    new_lemmer.cpp
)

PEERDIR(
    kernel/search_types
    library/cpp/containers/comptrie
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/lemmer/dictlib
    kernel/lemmer/new_engine/binary_dict
    kernel/lemmer/registry
)

END()
