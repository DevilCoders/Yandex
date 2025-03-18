LIBRARY()

OWNER(velavokr)

SRCS(
    coded_blob.cpp
    coded_blob_builder.cpp
    coded_blob_array.cpp
    coded_blob_array_builder.cpp
    coded_blob_trie.cpp
    coded_blob_trie_builder.cpp
    coded_blob_simple_builder.cpp
)

PEERDIR(
    contrib/libs/snappy
    library/cpp/codecs
    library/cpp/on_disk/coded_blob/common
    library/cpp/on_disk/coded_blob/keys
    library/cpp/succinct_arrays
)

END()
