PY2_LIBRARY()

OWNER(
    g:antirobot
)

PY_SRCS(
    nile_tools.py
    stream_converters.py
    event_log_reader.py
)

PEERDIR(
    statbox/nile
    antirobot/scripts/antirobot_eventlog
    antirobot/idl
    library/python/resource
)

RESOURCE(
    antirobot/idl/antirobot.ev /antirobot_ev
)

END()
