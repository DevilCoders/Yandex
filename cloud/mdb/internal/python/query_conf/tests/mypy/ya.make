PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    # This target is mandatory, it does all the job
    library/python/testing/types_test/py3
    cloud/mdb/internal/python/query_conf
)

TEST_SRCS(conftest.py)

# Since mypy is rather slow it could be a good idea to force test
# to have MEDIUM or even LARGE size, and increase timeout.
SIZE(MEDIUM)

TIMEOUT(600)

END()
