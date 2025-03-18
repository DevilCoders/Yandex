PROGRAM()

OWNER(velavokr)

SRCS(
    dater_server.cpp
)

PEERDIR(
    kernel/segmentator
    kernel/segutils
    kernel/tarc/disk
    library/cpp/deprecated/dater_old
    library/cpp/getopt
    library/cpp/http/fetch
    library/cpp/http/server
    quality/deprecated/Misc
    tools/segutils/segcommon
)

ALLOCATOR(LF)

END()
