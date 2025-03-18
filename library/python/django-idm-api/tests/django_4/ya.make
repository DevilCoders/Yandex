PY3TEST()

OWNER(g:tools-python)

PEERDIR(
    library/python/django-idm-api/tests

    contrib/python/django/django-4
)

ENV(DJANGO_SETTINGS_MODULE=django_idm_api.tests.settings)

END()
