PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(0.22)

PEERDIR(
    contrib/python/django/django-1.11
    contrib/python/six
)

PY_SRCS(
    TOP_LEVEL
    django_russian/__init__.py
    django_russian/utils.py
    django_russian/templatetags/__init__.py
    django_russian/templatetags/russian.py
)

RESOURCE_FILES(
    PREFIX library/python/django_russian/

    django_russian/forms_adjective.txt
    django_russian/forms_noun.txt
    django_russian/forms_verb.txt

)

END()
