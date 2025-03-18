PY23_LIBRARY()

OWNER(levysotsky)

PEERDIR(
    library/python/type_info
    yt/python/client
)

TEST_SRCS(
    test_extension.py
    test_typing.py
    test_io.py
    helpers.py
)

END()
