PROGRAM()

OWNER(g:cs_dev)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/daemon
    library/cpp/getopt
    kernel/common_server/handlers
    kernel/common_server/service_monitor/handlers
    kernel/common_server/service_monitor/pods
    kernel/common_server/service_monitor/server
    kernel/common_server/service_monitor/web
    kernel/common_server/service_monitor/services
)

END()
