OWNER(g:mdb)

PY3TEST()

STYLE_PYTHON()

SIZE(MEDIUM)

PEERDIR(
    contrib/python/PyHamcrest
    contrib/python/pytest-mock
    cloud/mdb/dbaas_python_common
)

DATA(arcadia/cloud/mdb/dbaas_python_common)

TEST_SRCS(
    __init__.py
    conftest.py
    test_config.py
    test_dict.py
    test_retry.py
    test_tskv.py
)

END()
