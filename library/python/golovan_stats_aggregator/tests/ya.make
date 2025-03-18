PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/mock
    contrib/python/six
    contrib/python/PyHamcrest

    library/python/golovan_stats_aggregator

)

TEST_SRCS(
    unit/memory/__init__.py
    unit/memory/tests.py
    unit/__init__.py
    unit/uwsgi/__init__.py
    unit/uwsgi/tests.py
    unit/base/__init__.py
    unit/base/tests.py
)

END()

RECURSE(
    py2
    py3
)
