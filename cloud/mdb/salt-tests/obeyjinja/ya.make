PY3_PROGRAM()

STYLE_PYTHON()

OWNER(g:mdb)

PEERDIR(
    contrib/python/Jinja2
)

PY_SRCS(
    NAMESPACE cloud.mdb.salt_tests.obeyjinja
    fakesalt.py
    __main__.py
)

END()
