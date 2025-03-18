OWNER(sashbel soin08)

PY3_PROGRAM(test)

PY_SRCS(
    MAIN __init__.py
    udfs.py
)

PEERDIR(
    library/python/spyt/executable
    contrib/python/click
)

NO_CHECK_IMPORTS()

END()
