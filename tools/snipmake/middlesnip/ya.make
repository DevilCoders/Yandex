PROGRAM()

OWNER(divankov)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/base
    kernel/snippets/idl
    kernel/snippets/iface
    kernel/snippets/iface/archive
    kernel/snippets/strhl
    kernel/snippets/urlcut
    library/cpp/cgiparam
    library/cpp/http/io
    library/cpp/neh
    library/cpp/scheme
    library/cpp/string_utils/base64
    library/cpp/svnversion
    search/idl
    search/reqparam
    search/request/treatcgi
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
