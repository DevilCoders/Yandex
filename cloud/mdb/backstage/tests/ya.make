PY3TEST()

OWNER(g:mdb)

PEERDIR(
    library/python/django
    contrib/python/pytest-django

    cloud/mdb/backstage/settings
    cloud/mdb/backstage/lib
)


TEST_SRCS(
    conftest.py
    lib/test_params.py
    lib/test_helpers.py
)

NO_CHECK_IMPORTS()

END()
