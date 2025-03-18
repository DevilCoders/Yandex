PROGRAM()

OWNER(
    grechnik
    g:snippets
)

SRCS(
    forums.cpp
)

PEERDIR(
    kernel/indexer/dtcreator
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/indexer/tfproc
    kernel/recshell
    kernel/segutils
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/getopt
    library/cpp/html/pdoc
    tools/segutils/segcommon
    ysite/directtext/textarchive
)

END()

RECURSE(
    tests
)
