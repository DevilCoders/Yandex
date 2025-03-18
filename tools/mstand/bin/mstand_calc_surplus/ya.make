PY3_PROGRAM()

OWNER(
    g:mstand-online
)

PY_SRCS(
    __main__.py
    calculate_surplus.py
)

PEERDIR(
    quality/logs/mousetrack_lib/python
    quality/user_sessions/libra_arc
    statbox/nile
    quality/mstand_metrics/online_production/surplus_search
    quality/yaqlib/yaqlibenums
    quality/yaqlib/yaqutils
    tools/mstand/experiment_pool
    tools/mstand/mstand_enums
    tools/mstand/mstand_utils
    tools/mstand/session_squeezer
    tools/mstand/session_yt
    tools/mstand/user_plugins
)

END()
