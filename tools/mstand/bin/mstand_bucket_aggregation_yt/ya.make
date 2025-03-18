PY3_PROGRAM()

OWNER(
    g:mstand
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    quality/yaqlib/yaqutils
    tools/mstand/experiment_pool
    tools/mstand/mstand_utils
    tools/mstand/postprocessing/scripts
    yt/python/client
)

END()
