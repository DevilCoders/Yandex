UNITTEST_FOR(kernel/saas_trie)

OWNER(alexbykov)

PEERDIR(
    kernel/saas_trie/test_utils
)

SRCS(
    disk_trie_test.cpp
    duplicate_key_filter_test.cpp
    memory_trie_test.cpp
    test_helpers.cpp
    trie_iterator_chain_test.cpp
    trie_url_mask_iterator_test.cpp
)

END()
