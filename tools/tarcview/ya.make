PROGRAM()

ALLOCATOR(GOOGLE)

OWNER(leo)

SRCS(
    tarcview.cpp
)

PEERDIR(
    kernel/doom/chunked_wad
    kernel/indexer/faceproc
    kernel/tarc/disk
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/deprecated/dater_old
    library/cpp/getopt
    library/cpp/langs
    yweb/protos
)

END()
