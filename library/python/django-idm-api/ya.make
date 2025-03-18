PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/attrs
    library/python/yenv

    library/python/tvm2
)

PY_SRCS(
    TOP_LEVEL
    django_idm_api/__init__.py
    django_idm_api/compat.py
    django_idm_api/conf.py
    django_idm_api/constants.py
    django_idm_api/exceptions.py
    django_idm_api/forms.py
    django_idm_api/hooks.py
    django_idm_api/membership_views.py
    django_idm_api/middleware.py
    django_idm_api/models.py
    django_idm_api/permissions.py
    django_idm_api/settings.py
    django_idm_api/signals.py
    django_idm_api/urls.py
    django_idm_api/utils.py
    django_idm_api/validation.py
    django_idm_api/views.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
