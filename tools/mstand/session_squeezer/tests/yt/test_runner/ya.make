OWNER(g:mstand-online)

PY3_LIBRARY()

PEERDIR(
    tools/mstand/experiment_pool
    tools/mstand/session_squeezer
    tools/mstand/session_yt
)

PY_SRCS(
    NAMESPACE test_runner
    __init__.py
    conftest.py
    test_runner.py
)

END()
