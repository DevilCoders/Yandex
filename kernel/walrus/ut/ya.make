UNITTEST_FOR(kernel/walrus)

OWNER(aavdonkin)

PEERDIR(
    kernel/indexer/directindex
    kernel/indexer/posindex
    kernel/keyinv/indexfile
    kernel/keyinv/invkeypos
    library/cpp/digest/old_crc
    library/cpp/wordpos
    ysite/directtext/freqs
)

SRCS(
    advmerger_ut.cpp
    lfproc_ut.cpp
)

END()
