GO_PROGRAM(blockstore-volume-scanner)

OWNER(g:cloud-nbs)

SRCS(
    api.go
    main.go
    scan.go
)

PEERDIR(
    cloud/blockstore/private/api/protos
    cloud/blockstore/public/sdk/go/client
)

END()
