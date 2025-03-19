PROGRAM(daemon_tester)

OWNER(ivanmorozov)

PEERDIR(
    library/cpp/yconf
    kernel/daemon/config
    kernel/daemon/module
    kernel/daemon
)

SRCS(
    server.cpp
    config.cpp
    main.cpp
)

END()
