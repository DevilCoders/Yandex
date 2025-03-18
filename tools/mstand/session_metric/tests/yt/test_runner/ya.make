OWNER(g:mstand)

PY3_LIBRARY()

PEERDIR(
    tools/mstand/session_metric
    tools/mstand/experiment_pool
    tools/mstand/session_yt
)

PY_SRCS(
    NAMESPACE test_runner
    __init__.py
    test_runner.py
)

END()
