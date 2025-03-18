PROGRAM()

PEERDIR(
    kernel/segutils
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/getopt
    library/cpp/lcs
    library/cpp/tokenizer
    tools/segutils/segcommon
)

SRCS(
    arc_downloader.cpp
)

ALLOCATOR(LF)

END()
