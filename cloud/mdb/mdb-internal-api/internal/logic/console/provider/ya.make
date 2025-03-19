GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clusters.go
    console.go
    cud.go
    network.go
    stats.go
)

GO_TEST_SRCS(
    clusters_test.go
    cud_test.go
)

END()

RECURSE(gotest)
