PROGRAM(proxy)

OWNER(g:aapi)

ALLOCATOR(J)

PEERDIR(
    aapi/server
    aapi/lib/trace
    aapi/lib/sensors
    library/cpp/getopt
)

SRCS(main.cpp)

END()
