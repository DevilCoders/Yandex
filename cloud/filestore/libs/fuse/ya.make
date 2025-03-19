LIBRARY()

OWNER(g:cloud-nbs)

CFLAGS(
    -DFUSE_USE_VERSION=29
)

INCLUDE(${ARCADIA_ROOT}/cloud/filestore/libs/fuse/sources.inc)

SRCS(
    fuse.cpp
)

PEERDIR(
    contrib/libs/fuse
)

END()

RECURSE_FOR_TESTS(
    ut
)

IF (FUZZING)
    RECURSE_FOR_TESTS(
        fuzz
    )
ENDIF()
