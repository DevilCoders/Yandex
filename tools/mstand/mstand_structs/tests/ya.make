PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/mstand_structs/lamp_key_ut.py
    tools/mstand/mstand_structs/squeeze_versions_ut.py
)

PEERDIR(
    tools/mstand/mstand_structs
)

SIZE(SMALL)

END()
