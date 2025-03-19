GO_LIBRARY()

SRCS(
    client.go
    const.go
    interface.go
    types.go
)

END()

RECURSE(
    mocks
    testcmd
)
