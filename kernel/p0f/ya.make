BUILD_ONLY_IF(LINUX)

LIBRARY()

OWNER(g:balancer sereja589)

PEERDIR(
    kernel/p0f/bpf
    kernel/p0f/load
    kernel/p0f/format
    library/cpp/resource
)

SRCS(
    p0f.cpp
)

END()
