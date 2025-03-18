PY23_LIBRARY()

OWNER(
    ilyzhin
    g:matrixnet
)

PEERDIR(
    library/python/hnsw/lib
)

TEST_SRCS(
    test.py
)

END()
