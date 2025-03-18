PROGRAM()

OWNER(divankov)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/search_daemon_iface
    kernel/snippets/base
    kernel/snippets/idl
    kernel/snippets/iface
    kernel/snippets/iface/archive
    kernel/snippets/urlcut
    kernel/urlid
    library/cpp/json
    library/cpp/string_utils/base64
    library/cpp/string_utils/url
    search/idl
    search/session/compression
    tools/snipmake/common
    tools/snipmake/snipdat
)

END()
