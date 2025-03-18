PY2TEST()

OWNER(g:tools-python)

PEERDIR(
    library/python/granular_settings/tests
)

TEST_SRCS(
    app.py
    settings.py
    settings_from_envvar.py
    test_path.py
)

END()
