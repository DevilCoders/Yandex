PY23_LIBRARY()

OWNER(
    ivaxer
    bashaeva
)

PEERDIR(
    contrib/python/cachetools
    contrib/python/Flask
    contrib/python/Flask-Principal
    contrib/python/requests
    contrib/python/six
    library/python/deprecated/ticket_parser2
)

PY_SRCS(
    TOP_LEVEL
    flask_passport.py
    tvm.py
)

END()
