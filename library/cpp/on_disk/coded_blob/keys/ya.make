LIBRARY()

OWNER(velavokr)

SRCS(
    keys_comptrie.cpp
    keys_comptrie_builder.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/on_disk/coded_blob/common
)

END()
