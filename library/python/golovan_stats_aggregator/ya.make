PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(2.0.0)

PEERDIR(
    contrib/python/six
)

NO_CHECK_IMPORTS()

PY_SRCS(
    TOP_LEVEL
    golovan_stats_aggregator/exception.py
    golovan_stats_aggregator/memory.py
    golovan_stats_aggregator/uwsgi.py
    golovan_stats_aggregator/__init__.py
    golovan_stats_aggregator/base.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
