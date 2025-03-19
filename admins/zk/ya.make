GO_LIBRARY()

OWNER(
    skacheev
    g:music-sre
)

SRCS(
    client_interface.go
    zk.go
)

END()

RECURSE(
    cmd
    mocks
)
