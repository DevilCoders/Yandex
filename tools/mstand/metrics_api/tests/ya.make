PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/metrics_api/online_ut.py
)

PEERDIR(
    tools/mstand/metrics_api
)

SIZE(SMALL)

END()
