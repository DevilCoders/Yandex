PROGRAM()

PEERDIR(
    contrib/libs/libxml
    contrib/libs/pcre
    library/cpp/digest/md5
    library/cpp/http/fetch
    library/cpp/uri
    yweb/webxml
    yweb/xmldoc
)

ADDINCL(tools/trans_str)

SRCDIR(tools/trans_str)

SRCS(
    trans_str.cpp
    webxmltest.cpp
)

END()
