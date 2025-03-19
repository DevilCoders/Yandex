GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    dcs.go
    errors.go
)

END()

RECURSE(
    mock
    zookeeper
)
