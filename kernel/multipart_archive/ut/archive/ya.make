UNITTEST()

OWNER(iddqd
     nsofya)

PEERDIR(
    kernel/multipart_archive
)

SRCDIR(kernel/multipart_archive)

SRCS(
    multipart_archive_ut.cpp
    part_optimization_ut.cpp
    iterator_ut.cpp
)

SIZE(MEDIUM)

END()
