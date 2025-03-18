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
    precalc.py
)

PY_MAIN(precalc)

END()
