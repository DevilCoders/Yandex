OWNER(losev g:toloka-crowd-instruments)

PY3_PROGRAM()
PY_SRCS(__main__.py)
PEERDIR(
    library/python/toloka_client/src/tk_stubgen
    library/python/toloka-kit
)
END()
