PY23_LIBRARY()

OWNER(
    g:release_machine
)

PY_SRCS(
    __init__.py
    runner.py
)

PEERDIR(
    contrib/python/selenium
)

END()

