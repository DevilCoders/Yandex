GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    docker-process.go
    parse-opts.go
)

GO_TEST_SRCS(docker-process_test.go)

END()

RECURSE(gotest)
