UNITTEST_FOR(kernel/indexer/directindex)

OWNER(
    leo
    g:base
)

PEERDIR(
    kernel/indexer/posindex
    kernel/keyinv/indexfile
    kernel/walrus
    library/cpp/digest/md5
    library/cpp/digest/old_crc
    ysite/directtext/freqs
)

SRCS(
    directindex_disamb_ut.cpp
    directindex_ut.cpp
    directtarc_ut.cpp
)

END()
