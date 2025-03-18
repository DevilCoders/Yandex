import os
import socket


DEBUG = os.environ.get("DEBUG", "0") == "1"
DEBUG_SQL = os.environ.get("DEBUG_SQL", "0") == "1"
FORCE_USER = os.environ.get("FORCE_USER")
RANDOM_REQUESTS_TABLE = os.environ.get("RANDOM_REQUESTS_TABLE", "//home/antirobot/log-viewer/cbb_requests_services")
MAIN_CBB_HOST = os.environ.get("CBB_HOST", socket.gethostname())
CBB_HOSTS = os.environ.get("CBB_HOSTS", MAIN_CBB_HOST)

TVM_ALLOWED_CLIENTS_FROM = os.environ.get("TVM_ALLOWED_CLIENTS_FROM", "tvm_config.json")
TVM_API_CHECK = os.environ.get("TVM_API_CHECK", "1") == "1"

USE_TZ = True
TIME_ZONE = "Europe/Moscow"

# ========== Mail settings

EMAIL_HOST = "smtp.yandex-team.ru"
EMAIL_HOST_USER = "robot-ne-robot"
EMAIL_HOST_PASSWORD = os.environ.get("EMAIL_HOST_PASSWORD")
EMAIL_PORT = 587
EMAIL_TIMEOUT = 20  # seconds
EMAIL_USE_TLS = True
EMAIL_BACKEND = "django.core.mail.backends.smtp.EmailBackend"
FROM_EMAIL = "robot-ne-robot@yandex-team.ru"

# ========== Django settings

SITE_ID = 1

ALLOWED_HOSTS = [
    socket.gethostname()
] + CBB_HOSTS.split(",")

ROOT_URLCONF = "antirobot.cbb.cbb_django.project.urls"

FORM_RENDERER = "library.python.django.template.backends.forms_renderer.ArcadiaRenderer"

ROOT_DIR = os.environ.get("ROOT_DIR", ".")

SECRET_KEY = "MyLongAndSecretCorrectHorseBatteryStaple37Django"
STATIC_URL = "/static/"
STATICFILES_DIRS = [
    f"{ROOT_DIR}/static",
]

TEMPLATES = [
    {
        "BACKEND": "library.python.django.template.backends.arcadia.ArcadiaTemplates",
        "OPTIONS": {
            "debug": True,
            "loaders": [
                "library.python.django.template.loaders.resource.Loader",
                "library.python.django.template.loaders.app_resource.Loader",
            ],

            "context_processors": [
                "django.template.context_processors.debug",
                "django.template.context_processors.request",
                "django.contrib.auth.context_processors.auth",
                "django.contrib.messages.context_processors.messages",
            ],
        },
    }
]

INSTALLED_APPS = [
    "django.contrib.contenttypes",
    "django.contrib.auth",
    "django.contrib.staticfiles",
    "django.contrib.sites",
    "antirobot.cbb.cbb_django.cbb",
    "django_yauth",
    "django_idm_api",
    "django.contrib.messages",
]

MIDDLEWARE = [
    "django_idm_api.middleware.TVMMiddleware",
    "django_yauth.middleware.YandexAuthBackendMiddleware",
    "antirobot.cbb.cbb_django.cbb.middlewares.DbManagerMiddleware",
    "antirobot.cbb.cbb_django.cbb.middlewares.AuthCheckMiddleware",
    "django.middleware.locale.LocaleMiddleware",
    "django.middleware.common.CommonMiddleware",
    "django.contrib.sessions.middleware.SessionMiddleware",
    "django.contrib.messages.middleware.MessageMiddleware",
    "django.middleware.csrf.CsrfViewMiddleware",
]

DATABASES = {
    "default": {}
}
# =============================================================================

# ========== Logging

LOGGING = {
    "version": 1,
    "disable_existing_loggers": False,
    "handlers": {
        "console": {
            "class": "logging.StreamHandler",
        },
    },
    "root": {
        "handlers": ["console"],
        "level": os.getenv("DJANGO_LOG_LEVEL", "WARNING"),
    },
    "loggers": {
        "tvm_tickets": {
            "handlers": ["console"],
            "level": os.getenv("DJANGO_LOG_LEVEL", "INFO"),
            "propagate": False,
        },
    },
}

# ==================


# ========== Database settings
DB_ENGINE = "django.db.backends.postgresql_psycopg2"
PG_SSLMODE = os.environ.get("PG_SSLMODE")
PG_SSLROOTCERT = os.environ.get("PG_SSLROOTCERT")
DB_USER = os.environ.get("DB_USER", "cbb_user")
DB_PASSWORD = os.environ.get("DB_PASSWORD", "1")
DB_PORT = os.environ.get("DB_PORT", 15432)

MAIN_DATABASE = {
    "ENGINE":   DB_ENGINE,
    "NAME":     os.environ.get("MAIN_DB_NAME", "cbb"),
    "SCHEMA":   os.environ.get("MAIN_DB_SCHEMA", "cbb"),
    "USER":     DB_USER,
    "PASSWORD": DB_PASSWORD,
    "HOSTS":    os.environ.get("MAIN_DB_HOSTS", "localhost"),
    "PORT":     DB_PORT,
    "SSLMODE":  PG_SSLMODE,
}

HISTORY_DATABASE = {
    "ENGINE":   DB_ENGINE,
    "NAME":     os.environ.get("HISTORY_DB_NAME", "cbb_history"),
    "SCHEMA":   os.environ.get("HISTORY_DB_SCHEMA", "cbb_history"),
    "USER":     DB_USER,
    "PASSWORD": DB_PASSWORD,
    "HOSTS":    os.environ.get("HISTORY_DB_HOSTS", "localhost"),
    "PORT":     DB_PORT,
    "SSLMODE":  PG_SSLMODE,
}
# =============================================================================


# ========== ACL
ACTION_READ = 1
ACTION_GROUP_CREATE = 2
ACTION_GROUP_MODIFY = 3
ROLE_SUPERVISOR = "supervisor"

ROLES = {
    ROLE_SUPERVISOR: {
        "name": "Супервизор",
        "actions": [],  # supervisor can do everything
    },
    "admin": {
        "name": "Администратор группы",
        "actions": [ACTION_READ, ACTION_GROUP_CREATE, ACTION_GROUP_MODIFY],
    },
    "user": {
        "name": "Пользователь",
        "actions": [ACTION_READ],
    },
}
# =============================================================================


# ========== Cache settings
DEFAULT_CACHE = {
    "BACKEND": "django.core.cache.backends.memcached.MemcachedCache",
    "LOCATION": os.environ.get("MEMCACHED_LOCATION", "localhost:11211"),
}

CACHES = {
    "default": DEFAULT_CACHE,
    "api-cache": {
        **DEFAULT_CACHE,
        "KEY_PREFIX": "api-cache:",
        "TIMEOUT": 3600,
        "VERSION": 3,
    },
    "engine-manager": {
        **DEFAULT_CACHE,
        "KEY_PREFIX": "engine-manager:",
        "TIMEOUT": 3600,
    },
}
# =============================================================================


# ========== TVM django-idm-api
# https://wiki.yandex-team.ru/intranet/idm/api/django/

from django_idm_api.settings import *  # noqa

ANTIROBOT_TVM_CLIENT_ID = 2002152

IDM_TVM_CLIENT_ID = 2001600
IDM_INSTANCE = "production"
IDM_URL_PREFIX = "idm/"
TVM_CLIENT_ID = int(os.environ.get("TVM_CLIENT_ID", 2002238))
TVM_SECRET = os.environ.get("TVM_SECRET")
IDM_API_TVM_SETTINGS = {
    "client_id": TVM_CLIENT_ID,
    "allowed_clients": (TVM_CLIENT_ID, IDM_TVM_CLIENT_ID, ANTIROBOT_TVM_CLIENT_ID),
    "secret": TVM_SECRET,
}

ROLES_HOOKS = "antirobot.cbb.cbb_django.cbb.library.hooks.Hooks"
# =============================================================================


# ========== Staff for getting login info (for bypassing CORS)
STAFF_TOKEN = os.environ.get("STAFF_TOKEN")
# =============================================================================


# ========== ylock
YLOCK = {
    "backend": "yt",
    "proxy": os.environ.get("YLOCK_PROXY", "locke"),
    "prefix": os.environ.get("YLOCK_PREFIX", ""),
    "token": os.environ.get("YT_TOKEN", ""),
}

# =============================================================================

# ========== django_yauth:
from django_yauth.settings import *  # noqa
YAUTH_TYPE = "intranet"

from django_yauth.settings import blackbox
blackbox.BLACKBOX_URL = "https://blackbox.yandex-team.ru/blackbox/"
YAUTH_USE_SITES = False

# YAuth импортирует библиотеки не в той последовательности, что tvm,
# поэтому объекты клиентов не совпадают. Берём клиента прямо из tvm.
from tvm2 import protocol

YAUTH_TVM2_BLACKBOX_CLIENT = protocol.BlackboxClientId.ProdYateam
YAUTH_TVM2_CLIENT_ID = os.environ.get("TVM_CLIENT_ID")
YAUTH_TVM2_SECRET = os.environ.get("TVM_SECRET")
YAUTH_USE_TVM2_FOR_BLACKBOX = True
YAUTH_TVM2_ALLOWED_CLIENT_IDS = [
    IDM_TVM_CLIENT_ID,
    ANTIROBOT_TVM_CLIENT_ID,
]

if os.environ.get("DEBUG_AUTH", False):
    AUTHENTICATION_BACKENDS = [
        "django_yauth.authentication_mechanisms.dev.UserFromHttpHeaderAuthBackend",
        "django_yauth.authentication_mechanisms.dev.UserFromCookieAuthBackend",
        "django_yauth.authentication_mechanisms.dev.UserFromOsEnvAuthBackend",
    ]
else:
    AUTHENTICATION_BACKENDS = [
        "django_yauth.authentication_mechanisms.cookie.Mechanism",
        "django_yauth.authentication_mechanisms.oauth.Mechanism",
        "django_yauth.authentication_mechanisms.tvm.Mechanism",
    ]
# =============================================================================
