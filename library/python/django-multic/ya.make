PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(1.3.0)

PEERDIR(
    contrib/python/simplejson
    contrib/python/six
)

NO_CHECK_IMPORTS()

PY_SRCS(
    TOP_LEVEL
    multic/__init__.py
    multic/filters.py
    multic/forms.py
    multic/multic_json.py
    multic/resources.py
    multic/views.py
)

END()
