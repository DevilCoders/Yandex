GO_LIBRARY()

OWNER(g:mdb)

SRCS(discovery.go)

END()

RECURSE(
    conductorcache
    grpcdiscovery
    implementation
    mocks
)
