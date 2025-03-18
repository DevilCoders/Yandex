LIBRARY()

OWNER(pzuev)

SRCS(
    misc.cpp
    fetch.cpp
    zora.cpp
    direct.cpp
    rewritehtml.cpp
    linkmap.cpp
)

PEERDIR(
    dict/recognize/docrec
    kernel/recshell
    library/cpp/charset
    library/cpp/html/pdoc
    library/cpp/html/sanitize/css
    library/cpp/html/html5
    library/cpp/html/url
    library/cpp/html/zoneconf
    library/cpp/http/fetch
    library/cpp/http/io
    library/cpp/langs
    library/cpp/mime/types
    library/cpp/numerator
    library/cpp/html/pcdata
    library/cpp/threading/future
    library/cpp/uri
    yweb/robot/logel
    zora/client/lib
    yweb/webutil/url_fetcher_lib
    library/cpp/string_utils/quote
)

END()
