PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(5.3)

PEERDIR(
    contrib/python/six
    library/python/yenv
    library/python/blackbox
    library/python/tvm2
    library/python/django_template_common
)

NO_CHECK_IMPORTS(
    django_yauth.*
)

PY_SRCS(
    TOP_LEVEL
    django_yauth/authentication_mechanisms/__init__.py
    django_yauth/authentication_mechanisms/cookie.py
    django_yauth/authentication_mechanisms/dev.py
    django_yauth/authentication_mechanisms/tvm/request.py
    django_yauth/authentication_mechanisms/tvm/__init__.py
    django_yauth/authentication_mechanisms/base.py
    django_yauth/authentication_mechanisms/oauth.py
    django_yauth/templatetags/__init__.py
    django_yauth/templatetags/yauth.py
    django_yauth/user.py
    django_yauth/models.py
    django_yauth/util.py
    django_yauth/__init__.py
    django_yauth/context.py
    django_yauth/settings.py
    django_yauth/exceptions.py
    django_yauth/urls.py
    django_yauth/middleware.py
    django_yauth/views.py
    django_yauth/decorators.py
)

RESOURCE_FILES(
    PREFIX library/python/python-django-yauth
    django_yauth/templates/admin/login.html
    django_yauth/templates/login_form.html
    django_yauth/templates/yauth.html
    django_yauth/css/yauth.css
)

END()

RECURSE_FOR_TESTS(
    tests
)
