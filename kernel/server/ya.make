LIBRARY(server)

OWNER(anskor)

SRCS(
    config.cpp
    itsworker.cpp
    server.cpp
    serverstat.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/searchlog
    kernel/server/protos
    library/cpp/unistat
    library/cpp/digest/md5
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/svnversion
    library/cpp/threading/equeue
    library/cpp/threading/hot_swap
    apphost/api/service/cpp
    library/cpp/deprecated/atomic
)

END()
