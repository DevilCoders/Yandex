PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    library/python/tvm2
    contrib/python/vcrpy
    contrib/python/mock

)

IF(PYTHON3)

    TEST_SRCS(
        test_aio/conftest.py
        test_aio/test_aio_daemon_tvm2.py
        test_aio/test_aio_thread_tvm2.py
        conftest.py
        test_daemon_tvm2.py
        test_deploy_tvm2.py
        test_thread_tvm2.py
        test_import.py
    )
ELSE()

    TEST_SRCS(
        conftest.py
        test_daemon_tvm2.py
        test_deploy_tvm2.py
        test_thread_tvm2.py
        test_import.py
    )

ENDIF()

END()

RECURSE(
    py2
    py3
)
