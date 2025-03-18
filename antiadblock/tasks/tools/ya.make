PY23_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    stat.py
    misc.py
    const.py
    logger.py
    solomon.py
    juggler.py
    yt_utils.py
    calendar.py
    configs_api.py
    notifications.py
    common_configs.py
)

PEERDIR(
    contrib/python/requests
    library/python/tvmauth
    contrib/python/retry
    contrib/python/juggler_sdk
    library/python/statface_client
    contrib/python/pandas
)

END()
