GO_PROGRAM(blockstore-volume-repairer)

OWNER(g:cloud-nbs)

SRCS(
    api.go
    main.go
    repair.go
)

PEERDIR(
    cloud/blockstore/private/api/protos
    cloud/blockstore/public/sdk/go/client
)

END()
