PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(1.94)

PEERDIR(
    contrib/python/six
)

NO_CHECK_IMPORTS(
    django_template_common.templatetags.*
    django_template_common.utils
)

PY_SRCS(
    TOP_LEVEL
    django_template_common/__init__.py
    django_template_common/utils.py
    django_template_common/templatetags/__init__.py
    django_template_common/templatetags/template_common.py
)

RESOURCE_FILES(
    PREFIX library/python/django_template_common/
    django_template_common/templates/paginator.html
)

END()
