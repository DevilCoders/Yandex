LIBRARY()

OWNER(g:balancer sereja589)

LICENSE(GPL-2.0)

SRCS(
    bpf_load.c
)

ADDINCL(
    contrib/libs/libbpf/include
)

PEERDIR(
    contrib/libs/libbpf
)

END()
