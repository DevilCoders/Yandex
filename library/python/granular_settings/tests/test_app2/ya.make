PY2TEST()

OWNER(g:tools-python)

PEERDIR(
    library/python/granular_settings/tests
)

TEST_SRCS(
    test_app2.py
    app1/__init__.py
    app1/settings.py
    app2/__init__.py
    app2/settings.py
)

END()
