PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/requests
    contrib/python/celery/py2
    contrib/python/jsonfield
    contrib/python/djangorestframework
    library/python/yenv
    library/python/ylock
    library/python/ylog
    library/python/tvm2
)

PY_SRCS(
    TOP_LEVEL
    dir_data_sync/__init__.py
    dir_data_sync/admin.py
    dir_data_sync/dao.py
    dir_data_sync/dir_client.py
    dir_data_sync/errors.py
    dir_data_sync/injector.py
    dir_data_sync/logic.py
    dir_data_sync/models.py
    dir_data_sync/org_ctx.py
    dir_data_sync/permissions.py
    dir_data_sync/settings.py
    dir_data_sync/signals.py
    dir_data_sync/utils.py
    dir_data_sync/urls.py
    dir_data_sync/views.py
    dir_data_sync/tasks/__init__.py
    dir_data_sync/tasks/lock.py
    dir_data_sync/tasks/new_org.py
    dir_data_sync/tasks/sync_data.py
    dir_data_sync/management/__init__.py
    dir_data_sync/management/commands/__init__.py
    dir_data_sync/management/commands/init_org.py
    dir_data_sync/management/commands/sync_org_data.py
)

RESOURCE_FILES(
    PREFIX library/python/dir-sync/
    dir_data_sync/fixtures/initial_data.json
)

END()

RECURSE(
    dir_data_sync/migrations
)
