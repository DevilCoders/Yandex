PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE cli_tools
    __init__.py
    squeeze_yt.py
    squeeze_yt_single.py
    yt_clear_tool.py
)


PEERDIR(
    yt/python/client
    mapreduce/yt/python

    quality/logs/mousetrack_lib/python
    quality/yaqlib/yaqutils

    tools/mstand/adminka
    tools/mstand/experiment_pool
    tools/mstand/mstand_enums
    tools/mstand/mstand_utils
    tools/mstand/session_squeezer
    tools/mstand/session_yt
)

END()
