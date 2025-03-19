PY3_PROGRAM()
OWNER(the-nans)

PEERDIR(
    library/python/startrek_python_client
)

PY_SRCS(
    TOP_LEVEL
    MAIN dl_reply.py
    helpers.py

)

END()
