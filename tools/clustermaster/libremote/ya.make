LIBRARY()

OWNER(
    g:clustermaster
)

SRCS(
    remote.cpp
    http.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    ADDINCL library/cpp/http/fetch
    ADDINCL tools/clustermaster/common
    library/cpp/archive
    library/cpp/charset
    library/cpp/getopt/small
    library/cpp/svnversion
    library/cpp/string_utils/quote
    tools/clustermaster/libremote/fetch
)

ARCHIVE(
    NAME archive.inc
    cacert.pem
)

END()
