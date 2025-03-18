PY2TEST()

OWNER(
    pg
    borman
)

PEERDIR(
    library/python/yt
)

TEST_SRCS(
    test_ttl.py
    test_db.py
)

END()
