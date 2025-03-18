PROGRAM(hlconfig)

OWNER(
    g:clustermaster
    tolich
)

PEERDIR(
    library/cpp/getopt
    tools/clustermaster/master/lib
)

SRCS(
    hlconfig_main.cpp
)

END()
