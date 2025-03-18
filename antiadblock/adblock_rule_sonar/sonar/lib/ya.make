PY2_LIBRARY()

OWNER(g:antiadblock)

PY_SRCS(
    __init__.py
    config.py
    utils/__init__.py
    utils/parser.py
    utils/rule_parser.py
    utils/sonar_yt.py
    utils/sonar_logger.py
    utils/sonar_startrek.py
    utils/sonar_configs_api.py
)

PEERDIR(
    antiadblock/tasks/tools
    library/python/yt
    yt/yt/python/yt_yson_bindings
    library/python/startrek_python_client
    contrib/python/retry
)

END()
