OWNER(
    g:mstand-online
)

PY3_PROGRAM()

PY_SRCS(
    __main__.py
)

PEERDIR(
    quality/yaqlib/yaqutils

    tools/mstand/abt
    tools/mstand/adminka
    tools/mstand/experiment_pool
    tools/mstand/mstand_utils
)

END()
