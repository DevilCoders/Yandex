LIBRARY()

OWNER(divankov)

SRCS(
    askctx.cpp
    metahost.cpp
    usersessions.cpp
    xmlsearchin.xsyn
    xmlsearchin.cpp
)

PEERDIR(
    library/cpp/json
    library/cpp/xml/parslib
    library/cpp/uri
    search/idl
    library/cpp/http/io
    yweb/news/fetcher_lib
    library/cpp/cgiparam
    library/cpp/string_utils/quote
    library/cpp/string_utils/url
)

END()
