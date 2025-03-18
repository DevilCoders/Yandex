OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
)

IF(PYTHON2)
    PEERDIR(
        library/python/monitoring/solo/handlers/juggler/py2
        library/python/monitoring/solo/objects/juggler
    )
ENDIF()

IF(PYTHON3)
    PEERDIR(
        library/python/monitoring/solo/handlers/juggler/py3
        library/python/monitoring/solo/objects/juggler
    )
ENDIF()

END()

RECURSE(
    base
    py2
    py3
)
