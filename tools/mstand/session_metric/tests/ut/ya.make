PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/session_metric/conftest.py
    tools/mstand/session_metric/lamps_helpers_ut.py
    tools/mstand/session_metric/metric_calculator_ut.py
    tools/mstand/session_metric/metric_runner_ut.py
)

PEERDIR(
    tools/mstand/session_local
    tools/mstand/session_metric
)

DATA(
    arcadia/tools/mstand/session_metric/tests/ut/data/
)

SIZE(SMALL)

END()
