OWNER(esoloviev)

LIBRARY()

SRCS(
    calc.cpp
    writer.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/deprecated/split
    library/cpp/on_disk/aho_corasick
    library/cpp/on_disk/chunks
    library/cpp/string_utils/url
)

END()
