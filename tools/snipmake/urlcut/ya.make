OWNER(g:snippets)

PROGRAM(urlcut)

SRCS(
    run.cpp
)

PEERDIR(
    kernel/qtree/richrequest
    kernel/snippets/urlcut
    library/cpp/cgiparam
    library/cpp/charset
    library/cpp/getopt
    library/cpp/html/pcdata
    library/cpp/http/fetch
    library/cpp/langs
    library/cpp/string_utils/quote
)

END()
