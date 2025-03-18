OWNER(g:antirobot)

PROGRAM(cbb_fast)

SRCS(
    daemon.cpp
    database.cpp
    env.cpp
    server.cpp
    tvm.cpp
)

ALLOCATOR(HU)

PEERDIR(
    antirobot/cbb/cbb_fast/protos
    antirobot/lib
    contrib/libs/libpqxx
    kernel/server
    library/cpp/getopt
    library/cpp/getoptpb
    library/cpp/iterator
    library/cpp/json
    library/cpp/protobuf/json
    library/cpp/resource
    library/cpp/terminate_handler
    maps/libs/pgpool
    library/cpp/deprecated/atomic
)

RESOURCE(
    devtools/certs/cloud/postgresql_root.crt root.crt
)

END()
