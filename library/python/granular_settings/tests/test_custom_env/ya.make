PY2TEST()

OWNER(g:tools-python)

PEERDIR(
    library/python/granular_settings/tests
)

TEST_SRCS(
    test_settings.py
    app/__init__.py
    app/settings_b2b_extra_local.py
    app/settings_empty_env.py
    app/settings_extra_local_b2b.py
    app/settings_local_b2b_extra.py
)

END()
