OWNER(g:cloud-nbs)

GO_TEST()

SRCS(discovery.go)

PEERDIR(cloud/blockstore/config)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/recipes/local-kikimr/local-kikimr.inc)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

SIZE(MEDIUM)

END()
