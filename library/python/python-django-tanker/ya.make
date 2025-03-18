PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(1.7)

PEERDIR(
    contrib/python/lxml
    contrib/python/requests
)

PY_SRCS(
    TOP_LEVEL
    django_tanker/__init__.py
    django_tanker/api.py
    django_tanker/management/__init__.py
    django_tanker/management/commands/__init__.py
    django_tanker/management/commands/tankerdeletebranch.py
    django_tanker/management/commands/tankerdownload.py
    django_tanker/management/commands/tankerputbranch.py
    django_tanker/management/commands/tankerupload.py
    django_tanker/settings.py
    django_tanker/utils.py
)

END()
