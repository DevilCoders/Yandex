PY3TEST()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/django/django-2.2
    library/python/django-sform
)

TEST_SRCS(
    __init__.py
    conftest.py
    field_test.py
    fixtures.py
    form_test.py
    smoke_test.py
)

END()
