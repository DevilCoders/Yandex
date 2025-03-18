PROGRAM(worker)

OWNER(
    g:clustermaster
)

PEERDIR(
    library/cpp/cgiparam
    library/cpp/deprecated/fgood
    library/cpp/deprecated/split
    library/cpp/getopt
    library/cpp/http/server
    library/cpp/xml/encode
    tools/clustermaster/common
    tools/clustermaster/worker/lib
)

SRCS(
    worker_main.cpp
)

END()
