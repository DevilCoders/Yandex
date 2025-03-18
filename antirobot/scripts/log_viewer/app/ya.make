OWNER(g:antirobot)

PY2_LIBRARY()

PEERDIR(
    contrib/python/Flask
    contrib/python/Flask-WTF
    yt/python/client
    devtools/fleur/util
    antirobot/scripts/antirobot_eventlog
    antirobot/scripts/utils
    antirobot/scripts/access_log
    antirobot/scripts/log_viewer/config
)

PY_SRCS(
    accesslog_formatter.py
    data_fetcher.py
    eventlog_adapter.py
    log_eventlog.py
    misc.py
    search_access.py
    str_tools.py
    validate.py
    yt_cache.py
    calc_redirects.py
    domain_tools.py
    eventlog_formatter.py
    __init__.py
    log_market.py
    precalc_mgr.py
    search_events.py
    view.py
    yt_client.py
    calc_redirects_view.py
    errors.py
    forms.py
    log_accesslog.py
    log_mgrs.py
    precalc_view.py
    time_utl.py
)

RESOURCE(
    yweb/urlrules/areas.lst areas.lst
)

END()
