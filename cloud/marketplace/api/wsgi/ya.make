OWNER(g:cloud-marketplace)

PY3_PROGRAM(yc_marketplace)

PY_SRCS(
    MAIN main.py
    wsgi.py
)

PEERDIR(
    cloud/marketplace/api/yc_marketplace
    cloud/bitbucket/python-common
    contrib/python/uwsgi
)

END()
