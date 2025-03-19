PY3_PROGRAM()
OWNER(the-nans)

PEERDIR(
    library/python/startrek_python_client
)

PY_SRCS(
    TOP_LEVEL
    MAIN autoreply.py
    helpers.py

)

END()
