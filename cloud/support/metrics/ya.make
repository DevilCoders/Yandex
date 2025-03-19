PY3_PROGRAM(collector)

OWNER(sinkvey)

PEERDIR(
    contrib/python/requests
    contrib/python/prettytable
    contrib/python/PyYAML
    contrib/python/PyMySQL
    library/python/startrek_python_client
)

PY_SRCS(
    TOP_LEVEL
    MAIN collector.py

    metrics_collector/__init__.py
    metrics_collector/constants.py
    metrics_collector/error.py
    metrics_collector/database.py
    metrics_collector/helpers.py
    metrics_collector/objects/base.py
    metrics_collector/objects/comment.py
    metrics_collector/objects/issue.py
)

END()
