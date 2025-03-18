PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    library/python/granular_settings
)

RESOURCE_FILES(
    PREFIX library/python/granular_settings/tests/

    test_path/etc/myapp/001-main.conf
    test_app2/app1/settings/001-main.conf
    test_app2/app2/settings/001-main.conf
    test_app1/appA/settings/001-main.conf
    test_custom_env/app/settings/001-main.conf
    test_custom_env/app/settings/001-main.conf.development
    test_custom_env/app/settings/b2b/001-main.conf
    test_custom_env/app/settings/b2b/001-main.conf.development
    test_custom_env/app/settings/extra/001-main.conf
    test_custom_env/app/settings/extra/001-main.conf.development
    test_custom_env/app/settings/local/001-main.conf
    test_custom_env/app/settings/local/001-main.conf.development
)

END()

RECURSE(
    test_app1
    test_app2
    test_path
    test_custom_env
)
