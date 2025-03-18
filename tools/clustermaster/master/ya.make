PROGRAM(master)

OWNER(g:clustermaster)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/containers/sorted_vector
    library/cpp/deprecated/split
    library/cpp/deprecated/transgene
    library/cpp/digest/md5
    library/cpp/getopt
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/mime/types
    library/cpp/string_utils/quote
    library/cpp/uri
    tools/clustermaster/master/lib
    library/cpp/deprecated/atomic
)

SRCS(
    master_main.cpp
)

END()

RECURSE(
    graph-test
)
