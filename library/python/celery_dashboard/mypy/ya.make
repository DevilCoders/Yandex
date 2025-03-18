PY3TEST()

OWNER(
    g:news-bqn
)

PEERDIR(
    library/python/testing/types_test/py3
    library/python/celery_dashboard
)

TEST_SRCS(
    conftest.py
)

NO_CHECK_IMPORTS()

SIZE(MEDIUM)
TIMEOUT(600)

END()
