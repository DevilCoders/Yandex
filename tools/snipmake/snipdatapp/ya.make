PROGRAM()

OWNER(divankov)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/idl
    kernel/urlid
    library/cpp/cgiparam
    library/cpp/http/io
    library/cpp/json
    library/cpp/lcs
    library/cpp/string_utils/base64
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
    search/idl
    search/session/compression
    tools/snipmake/common
    tools/snipmake/reqrestr
    tools/snipmake/snipdat
)

SRCS(
    main.cpp
)

END()
