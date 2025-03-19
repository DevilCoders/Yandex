PROGRAM(blockstore-server)

OWNER(g:cloud-nbs)

ALLOCATOR(LF_DBG)

SRCDIR(${ARCADIA_ROOT}/cloud/blockstore/daemon)
INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/daemon/sources.inc)

PEERDIR(
    cloud/blockstore/libs/daemon
)

END()
