PY23_TEST()

OWNER(g:tools-python)

ENV(DJANGO_SETTINGS_MODULE=python_django_yauth.settings)

PEERDIR(
    library/python/python-django-yauth
    library/python/django
    contrib/python/mock
    contrib/python/pytest-django
    contrib/python/requests
    contrib/python/vcrpy
)

IF (PYTHON2)
    PEERDIR(
        contrib/python/django/django-1.11
    )
ELSE()
    PEERDIR(
        contrib/python/django/django-3
    )
ENDIF()

PY_SRCS(
    NAMESPACE python_django_yauth
    settings.py
)

TEST_SRCS(
    test_django_auth_backends.py
    test_django_auth_backends_dev.py
    tests.py
)

END()
