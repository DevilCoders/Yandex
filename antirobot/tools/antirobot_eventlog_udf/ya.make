YQL_PYTHON_UDF(antirobot_eventlog_udf)

OWNER(
    g:antirobot
)

REGISTER_YQL_PYTHON_UDF(
    NAME CustomPython2
    RESOURCE_NAME CustomPython2
)

PEERDIR(
    antirobot/scripts/antirobot_eventlog
)

END()
