OWNER(g:antirobot)

PY2_LIBRARY()

PY_SRCS(
    __init__.py
    ipaddr.py
    request.py
    splitter.py
    subnet.py
)

PEERDIR(
     devtools/fleur/imports/arcadia/null
     antirobot/scripts/utils
)

END()
