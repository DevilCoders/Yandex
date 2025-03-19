OWNER(g:cloud-dwh)

PY3_PROGRAM()

PEERDIR(
    cloud/dwh/lms
    library/python/nirvana
)

PY_SRCS(MAIN __init__.py)

END()
