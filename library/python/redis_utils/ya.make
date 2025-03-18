PY3_LIBRARY()

OWNER(stupidhobbit)

PY_SRCS(
    __init__.py
)

PEERDIR(
    contrib/python/redis
    contrib/python/pydantic
)

END()

RECURSE_FOR_TESTS(
    tests
)

RECURSE(recipe)
