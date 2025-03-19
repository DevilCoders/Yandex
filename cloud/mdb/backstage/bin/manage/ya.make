PY3_PROGRAM(backstage-manage)

OWNER(g:mdb)

PEERDIR(
    contrib/python/ipython
    library/python/django
    cloud/mdb/backstage/settings
)

PY_SRCS(
    __main__.py
)

NO_CHECK_IMPORTS()

END()
