PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/mock
    contrib/python/pytest-django
    library/python/django-idm-api
)

PY_SRCS(
    NAMESPACE django_idm_api.tests
    settings.py
    urls.py
    utils.py
)

TEST_SRCS(
    conftest.py
    test_add_remove_signals.py
    test_add_role.py
    test_get_all_roles.py
    test_get_roles.py
    test_hooks.py
    test_info.py
    test_memberships.py
    test_multitenancy.py
    test_remove_role.py
)

END()

RECURSE_FOR_TESTS(
    django_1
    django_2
    django_3
    django_4
)
