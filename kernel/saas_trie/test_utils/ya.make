LIBRARY()

OWNER(alexbykov)

PEERDIR(
    kernel/saas_trie
    library/cpp/testing/unittest
)

SRCS(
    fake_disk_trie.cpp
    test_utils.cpp
)

END()
