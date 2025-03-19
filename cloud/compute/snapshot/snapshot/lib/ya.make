GO_LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    facade.go
    gc.go
)

GO_TEST_SRCS(z_test.go)

END()

RECURSE(
    chunker
    convert
    directreader
    filewatcher
    globalauth
    gotest
    kikimr
    limits
    misc
    move
    nbd
    parallellimiter
    proxy
    server
    storage
    tasks
)
