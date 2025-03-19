GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    healthiness.go
    result.go
)

END()

RECURSE(
    healthbased
    jugglerbased
    mocks
)
