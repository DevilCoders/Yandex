LIBRARY()

OWNER(
    ssmike
    g:base
)

PEERDIR(
    library/cpp/yson
    robot/jupiter/protos/extarc
    kernel/tarc/docdescr
    kernel/tarc/iface
)

SRCS(compressor.cpp)

END()

RECURSE_FOR_TESTS(ut)
