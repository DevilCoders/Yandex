UNITTEST_FOR(library/cpp/offroad/trie)

OWNER(
    elric
    g:base
)

SRCS(
    trie_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/offroad/test
    library/cpp/offroad/custom
)

SIZE(MEDIUM)

END()
