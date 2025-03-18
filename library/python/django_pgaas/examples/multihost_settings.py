# coding=utf-8

from django_pgaas import HostManager


# Returned by `yc mdb cluster ListHosts --clusterId <uuid>`
hosts = [
    {
        "name": "foo.db.yandex.net",
        "options": {"geo": "sas", "type": "postgresql"},
    },
    {
        "name": "bar.db.yandex.net",
        "options": {"geo": "man", "type": "postgresql"},
    },
    {
        "name": "baz.db.yandex.net",
        "options": {"geo": "vla", "type": "postgresql"},
    },
]

manager = HostManager.create_from_yc(hosts)


DATABASES = {
    'default': {
        'ENGINE': 'django_pgaas.backend',
        'HOST': manager.host_string,
        'PORT': 6432,
        'USER': '<secret-user>',
        'PASSWORD': '<secret-password>',
        'NAME': '<secret-db-name>',
        'OPTIONS': {'target_session_attrs': 'read-write'},
    },

    # N.B.: `slave` is not necessarily a read-only replica!
    # If both app instance and master are in the same DC,
    # `slave` will also point to master.

    'slave': {
        'ENGINE': 'django_pgaas.backend',
        'HOST': manager.host_string,
        'PORT': 6432,
        'USER': '<secret-user>',
        'PASSWORD': '<secret-password>',
        'NAME': '<secret-db-name>',
        'OPTIONS': {'target_session_attrs': 'any'},
    },
}

# Optional, if django-replicated is used
REPLICATED_DATABASE_SLAVES = ['slave']
