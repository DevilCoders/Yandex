PY3_PROGRAM()

OWNER(
    g:mstand-online
)

PY_SRCS(
   __main__.py
)

PEERDIR(
    quality/logs/mousetrack_lib/python
    quality/user_sessions/libra_arc
    quality/yaqlib/yaqutils
    tools/mstand/cli_tools
)

END()
