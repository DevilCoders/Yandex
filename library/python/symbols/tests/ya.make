PY2TEST()

OWNER(pg)

PEERDIR(
    library/python/symbols/libc
)

IF (NOT OS_WINDOWS)
    PEERDIR(
        library/python/symbols/uuid
    )
ENDIF()

TEST_SRCS(
    test_ctypes.py
)

END()
