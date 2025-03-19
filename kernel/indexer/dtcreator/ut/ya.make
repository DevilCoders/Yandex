UNITTEST()

OWNER(g:base)

PEERDIR(
    kernel/indexer/direct_text
    ADDINCL kernel/indexer/dtcreator
    library/cpp/charset
)

SRCS(
    directtext_ut.cpp
    lemcache_ut.cpp
)

END()
