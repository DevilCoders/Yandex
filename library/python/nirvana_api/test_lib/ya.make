PY23_LIBRARY()

OWNER(alb82 gotmanov mihajlova mstebelev)

PEERDIR(
    library/python/nirvana_api
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
