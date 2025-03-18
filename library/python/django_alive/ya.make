PY23_LIBRARY()

OWNER(g:tools-python)

VERSION(1.1.12)

PEERDIR(
    contrib/python/django/django-1.11
    contrib/python/tzlocal
    contrib/python/six
)

PY_SRCS(
    TOP_LEVEL
    django_alive/checkers/memcached.py
    django_alive/checkers/db.py
    django_alive/checkers/zookeeper.py
    django_alive/checkers/generic.py
    django_alive/checkers/mongodb.py
    django_alive/checkers/__init__.py
    django_alive/checkers/redis.py
    django_alive/checkers/celery.py
    django_alive/checkers/http.py
    django_alive/checkers/base.py
    django_alive/loading.py
    django_alive/tasks.py
    django_alive/models.py
    django_alive/migrations/__init__.py
    django_alive/migrations/0001_initial.py
    django_alive/migrations/0002_auto__del_unique_stamprecord_name_host__add_unique_stamprecord_name_ho.py
    django_alive/management/__init__.py
    django_alive/management/commands/alivegen.py
    django_alive/management/commands/alivecheck.py
    django_alive/management/commands/__init__.py
    django_alive/management/commands/base.py
    django_alive/management/commands/alivestate.py
    django_alive/__init__.py
    django_alive/stampers/db.py
    django_alive/stampers/mongodb.py
    django_alive/stampers/__init__.py
    django_alive/stampers/redis.py
    django_alive/stampers/base.py
    django_alive/utils.py
    django_alive/settings.py
    django_alive/middleware.py
    django_alive/state.py
    django_alive/views.py
)

RESOURCE_FILES(
    PREFIX library/python/python-django-alive

    django_alive/templates/alive/state_detail.html
    django_alive/templates/alive/gen/cron.tpl
    django_alive/templates/alive/gen/monrun.tpl
)

END()
