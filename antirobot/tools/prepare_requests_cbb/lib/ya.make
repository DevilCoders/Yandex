PY3_LIBRARY()

OWNER(
    g:antirobot
)

PY_SRCS(
    service_identifier.py
)

PEERDIR(
    library/python/resource
)

RESOURCE(
    antirobot/config/service_identifier.json service_identifier.json
)

END()

RECURSE_FOR_TESTS(
    test
)
