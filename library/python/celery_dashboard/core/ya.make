PY3_LIBRARY()

OWNER(g:sport)

PY_SRCS(
    __init__.py

    dashboard_handler_task.py
    redis_dashboard_io.py
    task_context.py
)

PEERDIR(
    contrib/python/celery/py3
    contrib/python/pydantic
    contrib/python/python-dotenv

    library/python/yenv
    library/python/celery_dashboard/serializers
)

NO_CHECK_IMPORTS()

END()
