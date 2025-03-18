PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/session_yt/squeeze_yt_ut.py
)

PEERDIR(
    tools/mstand/session_yt
    tools/mstand/session_squeezer
)

SIZE(SMALL)

END()
