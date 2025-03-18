PY23_LIBRARY()

OWNER(smosker g:tools-python)

VERSION(5.0)

IF(PYTHON3)

    PEERDIR(
        contrib/python/aiohttp
        contrib/python/tenacity
        library/python/blackbox
        contrib/python/requests
        library/python/tvmauth
        library/python/yenv
        library/python/ylog
    )

    PY_SRCS(
        TOP_LEVEL
        tvm2/aio/__init__.py
        tvm2/aio/base.py
        tvm2/aio/daemon_tvm2.py
        tvm2/aio/thread_tvm2.py
        tvm2/sync/__init__.py
        tvm2/sync/daemon_tvm2.py
        tvm2/sync/thread_tvm2.py
        tvm2/__init__.py
        tvm2/exceptions.py
        tvm2/protocol.py
        tvm2/qloud_tvm2.py
        tvm2/ticket.py
    )
ELSE()

    PEERDIR(
        library/python/blackbox
        contrib/python/requests
        library/python/tvmauth
        library/python/yenv
        library/python/ylog
    )

    PY_SRCS(
        TOP_LEVEL
        tvm2/sync/__init__.py
        tvm2/sync/daemon_tvm2.py
        tvm2/sync/thread_tvm2.py
        tvm2/__init__.py
        tvm2/exceptions.py
        tvm2/protocol.py
        tvm2/qloud_tvm2.py
        tvm2/ticket.py
    )

ENDIF()

END()
