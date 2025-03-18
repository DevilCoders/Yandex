PROGRAM()

OWNER(divankov)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    kernel/search_daemon_iface
    library/cpp/json
    search/idl
    tools/snipmake/common
    tools/snipmake/snipdat
    library/cpp/string_utils/url
)

END()
