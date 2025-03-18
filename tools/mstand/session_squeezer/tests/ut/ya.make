PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/session_squeezer/services_ut.py
    tools/mstand/session_squeezer/suggest_ut.py
)

PEERDIR(
    quality/yaqlib/yaqutils
    tools/mstand/session_squeezer
    tools/mstand/experiment_pool
)

SIZE(SMALL)

END()
