UNITTEST_FOR(cloud/blockstore/support/CLOUDINC-1800/tools/cleanup-xfs)

OWNER(g:cloud-nbs)

SRCS(
    cleanup_ut.cpp
    cleanup.cpp

    parser_ut.cpp
    parser.cpp
)

PEERDIR(
    cloud/storage/core/libs/common
)

DATA(
    arcadia/cloud/blockstore/support/CLOUDINC-1800/tools/cleanup-xfs/ut/sb.txt
    arcadia/cloud/blockstore/support/CLOUDINC-1800/tools/cleanup-xfs/ut/freesp.txt
)

END()
