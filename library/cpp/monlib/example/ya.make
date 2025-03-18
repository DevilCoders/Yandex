PROGRAM(monlib-example)

OWNER(g:solomon)

NO_WSHADOW()

SRCS(
    monlib_example.cpp
)

PEERDIR(
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/service/auth/tvm
    library/cpp/monlib/service
    library/cpp/getopt
)

END()
