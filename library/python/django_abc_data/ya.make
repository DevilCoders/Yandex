PY23_LIBRARY()

OWNER(g:python-contrib)

VERSION(0.2.13)

PEERDIR(
    contrib/python/django-appconf
    contrib/python/django-closuretree
    contrib/python/six
    intranet/sync_tools
    library/python/ids
    library/python/tvmauth
)

PY_SRCS(
    TOP_LEVEL
    django_abc_data/__init__.py
    django_abc_data/admin.py
    django_abc_data/compat.py
    django_abc_data/conf.py
    django_abc_data/core.py
    django_abc_data/generator.py
    django_abc_data/lock.py
    django_abc_data/management/__init__.py
    django_abc_data/management/commands/__init__.py
    django_abc_data/management/commands/sync_abc_services.py
    django_abc_data/migrations/0001_initial.py
    django_abc_data/migrations/0002_auto_20190826_1315.py
    django_abc_data/migrations/__init__.py
    django_abc_data/models.py
    django_abc_data/signals.py
    django_abc_data/syncer.py
    django_abc_data/tasks.py
    django_abc_data/tests/__init__.py
    django_abc_data/tests/conftest.py
    django_abc_data/tests/settings.py
    django_abc_data/tests/test_sync.py
    django_abc_data/tests/utils.py
    django_abc_data/tvm.py
)

END()
