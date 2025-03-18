PROGRAM()

SRCS(
    printurls.cpp
)

PEERDIR(
    kernel/doc_remap
    kernel/keyinv/indexfile
    kernel/mirrors
    kernel/tarc/iface
    kernel/uri/norm
    library/cpp/charset
    library/cpp/containers/comptrie
    library/cpp/getopt
    library/cpp/microbdb
    library/cpp/string_utils/url
    quality/urllib
    search/meta/generic
    ysite/yandex/common
    yweb/robot/dbscheeme
)

END()
