GO_LIBRARY()

OWNER(g:mdb)

PEERDIR(kikimr/public/sdk/go/persqueue)

SRCS(
    config.go
    writer.go
)

END()
