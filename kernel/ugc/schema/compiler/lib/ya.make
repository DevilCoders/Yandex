OWNER(
    g:ugc
    stakanviski
)

PY3_LIBRARY()

PY_SRCS(
    generator.py
    parser.py
    __init__.py
)

PEERDIR(
    contrib/python/pyparsing
)

END()

RECURSE_FOR_TESTS(ut)
