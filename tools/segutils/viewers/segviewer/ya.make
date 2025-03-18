PROGRAM()

OWNER(
    velavokr
)

SRCS(
    segserverscommon.cpp
    segviewer.cpp
)

PEERDIR(
    kernel/segutils
    library/cpp/getopt
    library/cpp/html/entity
    library/cpp/http/server
    library/cpp/html/pcdata
    library/cpp/svnversion
    tools/segutils/segcommon
    library/cpp/string_utils/quote
)

END()
