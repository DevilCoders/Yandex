OWNER(g:cloud-nbs)

GO_LIBRARY()

SRCS(
    client.go
    factory.go
    interface.go
    session.go
)

END()

RECURSE(
    config
)

RECURSE_FOR_TESTS(
    mocks
    tests
)
