PY2TEST()

OWNER(g:tools-python)

PEERDIR(
    library/python/granular_settings/tests
)

TEST_SRCS(
    test_app_1.py
    appA/__init__.py
    appA/settings.py
)

END()
