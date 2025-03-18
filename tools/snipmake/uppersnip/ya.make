PROGRAM()

OWNER(divankov)

PEERDIR(
    contrib/libs/protobuf
    kernel/qtree/richrequest
    kernel/search_daemon_iface
    kernel/snippets/base
    kernel/snippets/config
    kernel/snippets/hits
    kernel/snippets/idl
    kernel/snippets/iface
    kernel/snippets/iface/archive
    kernel/snippets/strhl
    kernel/snippets/urlcut
    library/cpp/cgiparam
    library/cpp/json
    library/cpp/neh
    library/cpp/svnversion
    search/idl
    search/session/compression
    tools/snipmake/argv
    tools/snipmake/protosnip
    tools/snipmake/snipdat
    tools/snipmake/stopwordlst
)

SRCS(
    main.cpp
)

END()
