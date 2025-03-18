OWNER(
    g:solo
)

PY23_TEST()

TEST_SRCS(
    test_solomon_object.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/v2
    contrib/python/jsonobject
)

END()
