UNITTEST_FOR(cloud/blockstore/libs/discovery)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/libs/diagnostics
)

SIZE(MEDIUM)
TIMEOUT(600)

SRCS(
    balancing_ut.cpp
    ban_ut.cpp
    discovery_ut.cpp
    fetch_ut.cpp
    healthcheck_ut.cpp
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
)

END()
