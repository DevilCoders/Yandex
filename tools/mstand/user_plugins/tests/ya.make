PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/user_plugins/plugin_container_ut.py
    tools/mstand/user_plugins/plugin_key_ut.py
)

PEERDIR(
    tools/mstand/user_plugins
)

DATA(
    arcadia/tools/mstand/user_plugins/plugin_tests.py
)

SIZE(SMALL)

END()
