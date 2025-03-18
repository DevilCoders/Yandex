OWNER(pg)

PEERDIR(
    library/python/guid
)

SRCDIR(
    library/python/guid/test/lib
)

TEST_SRCS(
    test_at_fork.py
    test_native_str.py
)

DEPENDS(
    library/python/guid/at_fork_test
)
