PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    errors.py
    test_configs.py
    yt.py
    arg_parser.py
)

PEERDIR(
    yt/python/client
)


END()

