PY3_PROGRAM(celery_dashboard)

OWNER(g:sport)

PY_SRCS(
    __init__.py

    MAIN main.py
)

PEERDIR(
    contrib/python/celery/py3
    contrib/python/pydantic
    contrib/python/python-dotenv
    contrib/python/uvicorn

    library/python/redis_utils

    library/python/celery_dashboard/api
    library/python/celery_dashboard/core
    library/python/celery_dashboard/serializers
)

NO_CHECK_IMPORTS()

END()

RECURSE_FOR_TESTS(mypy)
