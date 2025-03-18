PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    library/python/blackbox
)

TEST_SRCS(
    test_timeout.py
    test_httperrors.py
    test_normal.py
    test_normal_json.py
)

END()

RECURSE(
    py2
    py3
)
