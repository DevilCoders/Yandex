PY3TEST()

OWNER(g:cloud-nbs)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/support/CLOUDINC-1800/tools/gen_used_blocks_map
)

PEERDIR(
)

DATA(
    arcadia/cloud/blockstore/support/CLOUDINC-1800/tools/gen_used_blocks_map/tests/data
)

END()
