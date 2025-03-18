PROGRAM(vcs)

OWNER(g:aapi)

STRIP()

PEERDIR(
    aapi/client
    aapi/fuse_client
    aapi/lib/hg
    library/cpp/getopt
)

SRCS(main.cpp)

END()
