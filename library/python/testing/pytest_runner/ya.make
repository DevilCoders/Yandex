PY23_LIBRARY()

OWNER(
    g:yatest
    dmitko
)

PY_SRCS(
    conftest.py
    trace.py
    junit.py
    runner.py
)

PEERDIR(
    devtools/ya/test/const
)

END()

RECURSE_FOR_TESTS(
   test
   example
)
