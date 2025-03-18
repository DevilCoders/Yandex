PROGRAM()

SRCS(
    dindex_tool.cpp
)

PEERDIR(
    kernel/groupattrs
    kernel/groupattrs/creator
    kernel/indexer/directindex
    kernel/indexer/posindex
    kernel/keyinv/indexfile
    kernel/lemmer
    kernel/tarc/iface
    kernel/walrus
    library/cpp/charset
    library/cpp/getopt
    ysite/directtext/freqs
)

END()
