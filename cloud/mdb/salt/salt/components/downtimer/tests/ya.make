OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

PEERDIR(
    cloud/mdb/salt/salt/components/downtimer
)

TEST_SRCS(
    test_downtimer.py
)

END()

RECURSE_FOR_TESTS(
    mypy
)
