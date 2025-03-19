GO_LIBRARY()

OWNER(g:mdb)

SRCS(deployapi.go)

END()

RECURSE(
    deployutils
    mocks
    restapi
)
