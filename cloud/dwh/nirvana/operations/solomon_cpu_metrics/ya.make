OWNER(g:cloud-dwh)

PY3_PROGRAM()

PEERDIR(
    cloud/dwh/utils
    library/python/nirvana
    yt/python/client
    contrib/python/arrow
    contrib/python/dateutil
    contrib/python/tenacity

    cloud/dwh/clients/solomon
    cloud/dwh/utils
)

PY_SRCS(MAIN __init__.py)

END()
