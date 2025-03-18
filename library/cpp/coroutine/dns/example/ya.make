PROGRAM(dns_example)

OWNER(pg)

PEERDIR(
    library/cpp/coroutine/engine
    library/cpp/coroutine/dns
    library/cpp/gettimeofday
)

SRCS(
    main.cpp
)

END()
