OWNER(g:cloud-dwh)

PY3_PROGRAM()

PEERDIR(
    contrib/python/click
    contrib/python/colorlog

    cloud/dwh/nirvana/config
    cloud/dwh/nirvana/reactor
    cloud/dwh/utils

    cloud/dwh/nirvana/vh
    cloud/analytics/nirvana/vh
)

PY_SRCS(MAIN __init__.py)

END()
