PY23_LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/build_info
    library/python/build_info/interface
    contrib/python/future
)

PY_SRCS(
    __init__.py
    __build_info.pyx
)

IF (PYTHON2)
    PEERDIR(
        library/python/build_info/py2
    )
ELSE()
    PEERDIR(
        library/python/build_info/py3
    )
ENDIF()

END()

RECURSE(
    interface
    py2
    py3
)
