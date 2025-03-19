from sentry.conf.server import *

import os.path
import socket

import ldap
from django_auth_ldap.config import LDAPSearch, PosixGroupType

CONF_ROOT = os.path.dirname(__file__)

DATABASES = {
    'default': {
        'ENGINE': 'sentry.db.postgres',
        'NAME': '{{ salt['dbaas.pillar']('data:sentry:database:name') }}',
        'USER': '{{ salt['dbaas.pillar']('data:sentry:database:user') }}',
        'PASSWORD': '{{ salt['dbaas.pillar']('data:sentry:database:password') }}',
        'HOST': '{{ salt['dbaas.pillar']('data:sentry:database:host') }}',
        'PORT': '{{ salt['dbaas.pillar']('data:sentry:database:port') }}',
        'CONN_MAX_AGE': 600,
        'OPTIONS': {
            'keepalives': 1,
            'keepalives_idle': 15,
            'keepalives_interval': 5,
            'keepalives_count': 3,
        },
        'AUTOCOMMIT': True,
        'ATOMIC_REQUESTS': False,
    }
}

SENTRY_USE_BIG_INTS = True

SENTRY_SINGLE_ORGANIZATION = True
DEBUG = False

SENTRY_CACHE = 'sentry.cache.redis.RedisCache'

REDIS_HOST = '{{ salt['dbaas.pillar']('data:sentry:cache:host') }}'
REDIS_PORT = {{ salt['dbaas.pillar']('data:sentry:cache:port') }}
REDIS_PASSWORD = '{{ salt['dbaas.pillar']('data:sentry:cache:password') }}'

BROKER_URL = 'redis://:{password}@{host}:{port}'.format(password=REDIS_PASSWORD, host=REDIS_HOST, port=REDIS_PORT)

SENTRY_RATELIMITER = 'sentry.ratelimits.redis.RedisRateLimiter'

SENTRY_BUFFER = 'sentry.buffer.redis.RedisBuffer'

SENTRY_QUOTAS = 'sentry.quotas.redis.RedisQuota'

SENTRY_TSDB = 'sentry.tsdb.redis.RedisTSDB'

SENTRY_DIGESTS = 'sentry.digests.backends.redis.RedisBackend'

SECURE_PROXY_SSL_HEADER = ('HTTP_X_FORWARDED_PROTO', 'https')
SESSION_COOKIE_SECURE = True
CSRF_COOKIE_SECURE = True

SENTRY_WEB_OPTIONS = {
    'workers': 16,
    'buffer-size': 524288,
    'protocol': 'uwsgi',
}

AUTH_LDAP_SERVER_URI = 'ldaps://ldap-dev.yandex.net:636'
AUTH_LDAP_GLOBAL_OPTIONS = {ldap.OPT_X_TLS_REQUIRE_CERT: ldap.OPT_X_TLS_NEVER}

AUTH_LDAP_USER_SEARCH = LDAPSearch(
    'ou=people,dc=yandex,dc=net',
    ldap.SCOPE_SUBTREE,
    '(uid=%(user)s)',
)

AUTH_LDAP_GROUP_SEARCH = LDAPSearch(
    "ou=groups,dc=yandex,dc=net",
    ldap.SCOPE_SUBTREE,
    "(cn=dpt_yandex_exp_9053_9308)"
)
AUTH_LDAP_GROUP_TYPE = PosixGroupType()
AUTH_LDAP_FIND_GROUP_PERMS = False
AUTH_LDAP_CACHE_GROUPS = True
AUTH_LDAP_BIND_AS_AUTHENTICATING_USER = True
AUTH_LDAP_GROUP_CACHE_TIMEOUT = 3600
AUTH_LDAP_REQUIRE_GROUP = 'cn=dpt_yandex_exp_9053_9308,ou=groups,dc=yandex,dc=net'
AUTH_LDAP_DENY_GROUP = None

AUTH_LDAP_USER_ATTR_MAP = {
    'name': 'cn',
    'email': 'mail'
}

SENTRY_MANAGED_USER_FIELDS = ('first_name', 'last_name', 'password',)
AUTH_LDAP_DEFAULT_SENTRY_ORGANIZATION = 'Sentry'
AUTH_LDAP_SENTRY_ORGANIZATION_ROLE_TYPE = 'admin'
AUTH_LDAP_SENTRY_ORGANIZATION_GLOBAL_ACCESS = True
AUTH_LDAP_SENTRY_SUBSCRIBE_BY_DEFAULT = False

AUTHENTICATION_BACKENDS = AUTHENTICATION_BACKENDS + (
    'sentry_ldap_auth.backend.SentryLdapBackend',
)

SENTRY_OPTIONS.update({
    'redis.clusters': {
        'default': {
            'hosts': {0: {'host': REDIS_HOST, 'port': REDIS_PORT, 'password': REDIS_PASSWORD}},
        },
    },
    'system.secret-key': '{{ salt['dbaas.pillar']('data:sentry:secret_key') }}',
    'system.admin-email': 'mdb-cc@yandex-team.ru',
    'mail.backend': 'django.core.mail.backends.smtp.EmailBackend',
    'mail.host': 'localhost',
    'mail.password': '',
    'mail.username': '',
    'mail.port': 25,
    'mail.use-tls': False,
    'mail.from': 'sentry@sentry.db.yandex-team.ru',
    'filestore.backend': 's3',
    'filestore.options': {
        'access_key': '{{ salt['dbaas.pillar']('data:sentry:s3:access_key') }}',
        'secret_key': '{{ salt['dbaas.pillar']('data:sentry:s3:secret_key') }}',
        'bucket_name': '{{ salt['dbaas.pillar']('data:sentry:s3:bucket') }}',
        'endpoint_url': '{{ salt['dbaas.pillar']('data:sentry:s3:url') }}',
    },
})
