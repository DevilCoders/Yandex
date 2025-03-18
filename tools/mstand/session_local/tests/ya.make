PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/session_local/mr_merge_local_ut.py
)

PEERDIR(
    tools/mstand/session_local
)

SIZE(SMALL)

END()
