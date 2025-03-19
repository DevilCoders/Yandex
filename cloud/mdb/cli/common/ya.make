OWNER(g:mdb)

PY3_LIBRARY()

STYLE_PYTHON()

PY_SRCS(
    formatting.py
    parameters.py
    prompts.py
    utils.py
    yaml.py
)

PEERDIR(
    contrib/libs/grpc/src/python/grpcio_status
    contrib/python/click
    contrib/python/dateutil
    contrib/python/deepdiff
    contrib/python/humanfriendly
    contrib/python/psycopg2
    contrib/python/Pygments
    contrib/python/PyYAML
    contrib/python/tabulate
    contrib/python/termcolor
)

END()

RECURSE(
    tests
)
