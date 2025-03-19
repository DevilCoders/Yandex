GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    context.go
    docker_composer.go
    network.go
    retry.go
    uuid.go
)

GO_XTEST_SRCS(context_test.go)

END()

RECURSE(
    appwrap
    gotest
    intapi
    notifieremulator
    workeremulation
)
