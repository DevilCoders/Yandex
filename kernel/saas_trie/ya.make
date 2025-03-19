LIBRARY()

OWNER(
    alexbykov
)

SRCS(
    config.cpp
    memory_trie.cpp
    disk_io.cpp
    disk_trie.cpp
    disk_trie_builder.cpp
    duplicate_key_filter.cpp
    trie_complex_key_iterator.cpp
    trie_iterator_chain.cpp
    trie_url_mask_iterator.cpp
)

PEERDIR(
    kernel/hosts/owner
    kernel/saas_trie/idl
    library/cpp/containers/comptrie
    library/cpp/logger/global
    library/cpp/string_utils/url
)

END()
