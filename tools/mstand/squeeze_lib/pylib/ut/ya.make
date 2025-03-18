PY3TEST()

OWNER(g:mstand)

PEERDIR(
    tools/mstand/mstand_utils
    tools/mstand/squeeze_lib/pylib
    tools/mstand/session_yt
)

TEST_SRCS(
    test_bindings.py
)

END()
