UNITTEST()

OWNER(nsofya)

PEERDIR(
    kernel/multipart_archive
    kernel/multipart_archive/hash
    kernel/multipart_archive/queue
)

SRCDIR(kernel/multipart_archive)

SRCS(
    performance_ut.cpp
)

SIZE(LARGE)

TAG(
    ya:force_sandbox
    ya:perftest
    ya:fat
)

REQUIREMENTS(
    cpu:12
)

END()
