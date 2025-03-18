OWNER(
    g:antirobot
)

PY2_PROGRAM()

PEERDIR(
    antirobot/scripts/antirobot_eventlog
    antirobot/scripts/access_log
    antirobot/scripts/utils
    antirobot/scripts/log_viewer/app
    yt/python/client
)

PY_SRCS(
    TOP_LEVEL
    slow_search_run.py
)

PY_MAIN(slow_search_run)

END()
