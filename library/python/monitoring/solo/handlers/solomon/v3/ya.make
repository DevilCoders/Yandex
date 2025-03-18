OWNER(
    g:solo
)

PY23_LIBRARY()

PY_SRCS(
    __init__.py
)

IF(PYTHON2)
    PEERDIR(
        library/python/monitoring/solo/handlers/solomon/v3/base
        library/python/monitoring/solo/handlers/solomon/v3/py2
    )
ENDIF()

IF(PYTHON3)
    PEERDIR(
        library/python/monitoring/solo/handlers/solomon/v3/base
        library/python/monitoring/solo/handlers/solomon/v3/py3
    )
ENDIF()

END()

RECURSE(
    base
    py2
    py3
)
