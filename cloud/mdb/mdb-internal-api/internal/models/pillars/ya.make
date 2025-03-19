GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    cluster.go
    pillars.go
    target_pillar.go
    utils.go
)

GO_TEST_SRCS(utils_test.go)

END()

RECURSE(gotest)
