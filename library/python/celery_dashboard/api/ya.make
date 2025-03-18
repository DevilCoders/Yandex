PY3_LIBRARY()

OWNER(g:sport)

PY_SRCS(
    __init__.py

    app.py

    tasks/__init__.py
    tasks/models.py
    tasks/routes.py
)

PEERDIR(
    contrib/python/fastapi
    contrib/python/pydantic
    contrib/python/python-dotenv

    library/python/celery_dashboard/core
    library/python/celery_dashboard/serializers
)

NO_CHECK_IMPORTS()

END()
