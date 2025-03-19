GO_LIBRARY()

OWNER(g:mdb)

PEERDIR(infra/tcp-sampler/pkg/porto)

SRCS(porto.go)

END()

RECURSE(
    mocks
    network
    runner
)
