PY23_LIBRARY()

OWNER(
    dshmatkov
)

VERSION(2.1.0)

PEERDIR(
    contrib/python/requests
    library/python/yenv
)

PY_SRCS(
    TOP_LEVEL
    blackboxer/__init__.py
    blackboxer/__version__.py
    blackboxer/blackboxer.py
    blackboxer/environment.py
    blackboxer/exceptions.py
    blackboxer/utils.py
)

IF (PYTHON3)
    PEERDIR(
        contrib/python/aiohttp
        contrib/python/tenacity
    )

    PY_SRCS(
        TOP_LEVEL
        blackboxer/_async.py
    )
ENDIF()

END()

RECURSE_FOR_TESTS(
    tests
)
