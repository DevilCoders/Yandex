UNITTEST()

OWNER(nsofya)

PEERDIR(
    kernel/multipart_archive
    kernel/multipart_archive/hash
)

SRCDIR(kernel/multipart_archive)

SRCS(
    multipart_hash_ut.cpp
)

DEPENDS(
    kernel/multipart_archive/ut/data/accumulators
)

SIZE(MEDIUM)

END()
