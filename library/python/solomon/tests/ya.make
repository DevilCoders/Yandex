OWNER(g:solomon)

PY23_LIBRARY()

TEST_SRCS(
    __init__.py
    test_reporters.py
)

PEERDIR(
    library/python/solomon
    contrib/python/mock
)

END()

RECURSE(
    py2
    py3
)
