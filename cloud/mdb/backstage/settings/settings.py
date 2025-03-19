import metrika.pylib.config as lib_config

import cloud.mdb.backstage.lib.log as mod_log
import cloud.mdb.backstage.lib.installation as mod_installation


PG_DEFAULTS = {
    'engine': 'django.db.backends.postgresql_psycopg2',
    'disable_server_side_cursors': True,
    'conn_max_age': 600,
    'conn_timeout': 2,
    'atomic_requests': False,
    'options': {
        'target_session_attrs': 'read-write',
        'connect_timeout': 2,
        'keepalives': 1,
        'keepalives_idle': 15,
        'keepalives_interval': 5,
        'keepalives_count': 3,
    },
}


def get_pg_config(data):
    return {
        'NAME': data.name,
        'USER': data.user,
        'PASSWORD': data.password,
        'HOST': data.host,
        'PORT': data.port,
        'ENGINE': data.get('engine', PG_DEFAULTS['engine']),
        'ATOMIC_REQUESTS': data.get('atomic_requests', PG_DEFAULTS['atomic_requests']),
        'CONN_MAX_AGE': data.get('conn_max_age', PG_DEFAULTS['conn_max_age']),
        'DISABLE_SERVER_SIDE_CURSORS': data.get('disable_server_side_cursors', PG_DEFAULTS['disable_server_side_cursors']),
        'OPTIONS': {
            'target_session_attrs': data.get('target_session_attrs', PG_DEFAULTS['options']['target_session_attrs']),
            'connect_timeout': data.get('connect_timeout', PG_DEFAULTS['options']['connect_timeout']),
            'keepalives': data.get('keepalives', PG_DEFAULTS['options']['keepalives']),
            'keepalives_idle': data.get('keepalives_idle', PG_DEFAULTS['options']['keepalives_idle']),
            'keepalives_interval': data.get('keepalives_interval', PG_DEFAULTS['options']['keepalives_interval']),
            'keepalives_count': data.get('keepalives_count', PG_DEFAULTS['options']['keepalives_count']),
        },
    }

CONFIG = lib_config.get_yaml_config('backstage')

DEBUG = CONFIG.django_debug
INSTALLATION = mod_installation.INSTALLATIONS_MAP[CONFIG.installation]
INSTALLATIONS = []
for name in mod_installation.NAMES:
    if name in CONFIG.get('installations', mod_installation.NAMES):
        INSTALLATIONS.append(mod_installation.INSTALLATIONS_MAP[name])

LINKS = []
for link in CONFIG.get('links', []):
    LINKS.append(link)

LANGUAGE_CODE = 'en-us'
SITE_ID = 1
TIME_ZONE = 'UTC'
USE_I18N = False
USE_L10N = False
USE_TZ = True
ALLOWED_HOSTS = ['*']
USE_X_FORWARDED_HOST = True


SECRET_KEY = CONFIG.secret_key

ROOT_URLCONF = 'cloud.mdb.backstage.apps.main.urls'

STATIC_URL = '/static/'
STATIC_ROOT = 'staticfiles'

INSTALLED_APPS = [
    'django.contrib.auth',
    'django.contrib.messages',
    'django.contrib.sessions',
    'django.contrib.humanize',
    'django.contrib.staticfiles',
    'django.contrib.contenttypes',
    'crispy_forms',
    'cloud.mdb.backstage.lib',
    'cloud.mdb.backstage.apps.iam',
    'cloud.mdb.backstage.apps.main',
]

ENABLED_BACKSTAGE_APPS = set()

for app, app_config in CONFIG.apps.items():
    if app_config.enabled:
        ENABLED_BACKSTAGE_APPS.add(app)
        INSTALLED_APPS.append(f'cloud.mdb.backstage.apps.{app}')

DATABASE_ROUTERS = ['cloud.mdb.backstage.apps.main.routers.DBRouter']

DATABASES = {}
if CONFIG.no_database:
    DATABASES['default'] = {
        'ENGINE': 'django.db.backends.sqlite3',
    }
else:
    DATABASES['default'] = get_pg_config(CONFIG.database)

for app, app_config in CONFIG.apps.items():
    if app_config.enabled:
        DATABASES[f'{app}_db'] = get_pg_config(app_config.database)

DEFAULT_AUTO_FIELD = 'django.db.models.AutoField'

CRISPY_TEMPLATE_PACK = 'bootstrap3'
TEMPLATES = [
    {
        'BACKEND': 'library.python.django.template.backends.arcadia.ArcadiaTemplates',
        'OPTIONS': {
            'debug': True,
            'loaders': [
                'library.python.django.template.loaders.resource.Loader',
                'library.python.django.template.loaders.app_resource.Loader',
            ],
            'context_processors': [
                'django.template.context_processors.debug',
                'django.template.context_processors.request',
                'django.contrib.auth.context_processors.auth',
                'django.contrib.messages.context_processors.messages',
                'cloud.mdb.backstage.apps.main.context_processors.static',
                'cloud.mdb.backstage.apps.main.context_processors.from_settings',
            ],
        },
    },
]

FORM_RENDERER = 'library.python.django.template.backends.forms_renderer.ArcadiaRenderer'

STATICFILES_FINDERS = [
    'django.contrib.staticfiles.finders.FileSystemFinder',
    'django.contrib.staticfiles.finders.AppDirectoriesFinder',
    'library.python.django.contrib.staticfiles.finders.ArcadiaAppFinder'
]


def is_in_test():
    import django
    return hasattr(django, 'is_in_test') and django.is_in_test


if CONFIG.iam.enable_testing_auth or is_in_test():
    auth_middleware = 'cloud.mdb.backstage.apps.iam.middleware.IAMAuthTestMiddleware'
else:
    auth_middleware = 'cloud.mdb.backstage.apps.iam.middleware.IAMAuthMiddleware'


MIDDLEWARE = [
    'django.middleware.security.SecurityMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'cloud.mdb.backstage.lib.middleware.FixEmptyHostMiddleware',
    'django.middleware.common.CommonMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    auth_middleware,
    'cloud.mdb.backstage.apps.main.middleware.UsagePermissionMiddleware',
    'cloud.mdb.backstage.apps.main.middleware.UserProfileMiddleware',
    'cloud.mdb.backstage.apps.main.middleware.UserTimezoneMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    'django.middleware.clickjacking.XFrameOptionsMiddleware',
]

CRISPY_TEMPLATE_PACK = 'bootstrap3'

STATIC_ADDRESS = CONFIG.static.address

LOGGING = mod_log.LogConfig(app_name='backstage', config=CONFIG['logging']).as_dict()
