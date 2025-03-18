OWNER(g:yatool g:logbroker)

RECURSE(
    py2
    py3
)

PY23_LIBRARY()

PEERDIR(
    library/python/spack
)

TEST_SRCS(
    test_convert.py
)

END()

RECURSE(
    py2
    py3
)
