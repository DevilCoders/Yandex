# Django settings for ui project.

DEBUG = False
TEMPLATE_DEBUG = DEBUG

USER_AUTH_REQUIRED = True
ASK_ANG_TASKPOOL_AVAILABILITY = True


SERVER_EMAIL = 'steam-dev@yandex-team.ru'
SERVER_SMTP = 'outbound-relay.yandex.net'
ALLOW_EMAILS = True

ADMINS = (
    ('my34', 'my34@yandex-team.ru'),
    ('a-bocharov', 'a-bocharov@yandex-team.ru'),
    ('jelyscs', 'jelyscs@yandex-team.ru')
)

MANAGERS = ADMINS

DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.mysql',
        'NAME': 'steam',
        'USER': 'steam',
        'PASSWORD': 'cohEhch?Kd',
        'OPTIONS': {
            'init_command':
            'SET names utf8, storage_engine=INNODB, SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE',
        },

        'HOST': 'localhost',
        'PORT': '3306',
    }
}

CACHES = {
    'default': {
        'BACKEND': 'django.core.cache.backends.db.DatabaseCache',
        'LOCATION': 'steam_cache_table',
    }
}


# Hosts/domain names that are valid for this site; required if DEBUG is False
# See https://docs.djangoproject.com/en/1.5/ref/settings/#allowed-hosts
ALLOWED_HOSTS = ['steam.yandex-team.ru']

# Local time zone for this installation. Choices can be found here:
# http://en.wikipedia.org/wiki/List_of_tz_zones_by_name
# although not all choices may be available on all operating systems.
# In a Windows environment this must be set to your system time zone.
TIME_ZONE = 'Europe/Moscow'
DATE_FORMAT = 'd.m.Y'
TIME_FORMAT = 'H:i:s'
DATETIME_FORMAT = 'H:i:s d.m.Y'

DATETIME_INPUT_FORMATS = (
    '%H:%M:%S %d.%m.%Y',
)

DATE_INPUT_FORMATS = (
    '%d.%m.%Y',
)

# Language code for this installation. All choices can be found here:
# http://www.i18nguy.com/unicode/language-identifiers.html
LANGUAGE_CODE = 'en-us'

LANGUAGES = (
    ('en', 'English'),
    ('ru', 'Russian'),
)

SITE_ID = 1

# If you set this to False, Django will make some optimizations so as not
# to load the internationalization machinery.
USE_I18N = True

# If you set this to False, Django will not format dates, numbers and
# calendars according to the current locale.
USE_L10N = False

# If you set this to False, Django will not use timezone-aware datetimes.
USE_TZ = True

# Absolute filesystem path to the directory that will hold user-uploaded files.
# Example: "/var/www/example.com/media/"
MEDIA_ROOT = ''

# URL that handles the media served from MEDIA_ROOT. Make sure to use a
# trailing slash.
# Examples: "http://example.com/media/", "http://media.example.com/"
MEDIA_URL = ''

# Absolute path to the directory static files should be collected to.
# Don't put anything in this directory yourself; store your static files
# in apps' "static/" subdirectories and in STATICFILES_DIRS.
# Example: "/var/www/example.com/static/"
STATIC_ROOT = '/usr/share/pyshared/steam/static/'

# URL prefix for static files.
# Example: "http://example.com/static/", "http://static.example.com/"
STATIC_URL = '/static/'

# Additional locations of static files
STATICFILES_DIRS = (
    # Put strings here, like "/home/html/static" or "C:/www/django/static".
    # Always use forward slashes, even on Windows.
    # Don't forget to use absolute paths, not relative paths.
)

# List of finder classes that know how to find static files in
# various locations.
STATICFILES_FINDERS = (
    'django.contrib.staticfiles.finders.FileSystemFinder',
    'django.contrib.staticfiles.finders.AppDirectoriesFinder'
    #'django.contrib.staticfiles.finders.DefaultStorageFinder',
)

# Make this unique, and don't share it with anybody.
SECRET_KEY = 'a$yyg#dfy6ur!5tz6svhv$m2yo&^z@7yx__y=e*81_*)s5kao!'


MESSAGE_STORAGE = 'django.contrib.messages.storage.session.SessionStorage'

# List of callables that know how to import templates from various sources.
TEMPLATE_LOADERS = (
    'django.template.loaders.filesystem.Loader',
    'django.template.loaders.app_directories.Loader',
    #'django.template.loaders.eggs.Loader',
)

TEMPLATE_CONTEXT_PROCESSORS = (
    'django.contrib.auth.context_processors.auth',
    'django_yauth.context.yauth',
    'django.core.context_processors.i18n',
)

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'core.middleware.DBAliveMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.middleware.locale.LocaleMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    # 'django_yauth.middleware.YandexAuthMiddleware',
    'core.middleware.SteamAuthMiddleware',
    'core.middleware.HTTPSMiddleware',
    # 'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    # Uncomment the next line for simple clickjacking protection:
    # 'django.middleware.clickjacking.XFrameOptionsMiddleware',
)

ROOT_URLCONF = 'ui.urls'

# Python dotted path to the WSGI application used by Django's runserver.
WSGI_APPLICATION = 'ui.wsgi.application'

TEMPLATE_DIRS = (
    # Put strings here, like "/home/html/django_templates"
    # or "C:/www/django/templates".
    # Always use forward slashes, even on Windows.
    # Don't forget to use absolute paths, not relative paths.
)

SESSION_ENGINE = 'django.contrib.sessions.backends.cached_db'
SESSION_EXPIRE_AT_BROWSER_CLOSE = True

DEFAULT_INDEX_TABLESPACE = 'steam_indexes'
DEFAULT_TABLESPACE = 'steam_tables'

INSTALLED_APPS = (
    'django_yauth',
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.sites',
    'django.contrib.messages',
    'django.contrib.staticfiles',
    'django.contrib.admin',
    'django_template_common',

    # apps
    'core',
    'djcelery',
    'south',
    # 'lettuce.django',
    # 'django_nose',
)

LETTUCE_APPS = (
    'core',
)

TEST_RUNNER = 'django_nose.NoseTestSuiteRunner'

from datetime import timedelta
from celery.schedules import crontab

CELERY_ACKS_LATE = True
CELERYBEAT_SCHEDULER = 'djcelery.schedulers.DatabaseScheduler'
CELERYBEAT_SCHEDULE = {
    'timeout_estimations': {
        'task': 'core.tasks.timeout_estimations',
        'schedule': timedelta(minutes=30)
    },
    'temp_root_cleanup': {
        'task': 'core.tasks.temp_root_cleanup',
        'schedule': timedelta(hours=12)
    },
    'finish_taskpools': {
        'task': 'core.tasks.finish_taskpools',
        'schedule': timedelta(hours=1)
    },
    'send_stats': {
        'task': 'core.tasks.send_stats',
        'schedule': timedelta(hours=1)
    },
    'notify_assessors': {
        'task': 'core.tasks.notify_assessors',
        'schedule': timedelta(hours=3)
    },
    'email_aadmins': {
        'task': 'core.tasks.email_aadmins',
        'schedule': crontab(minute=0, hour=0, day_of_month='15'),
    },
}
BROKER_URL = 'amqp://steam:steam@localhost:5672/steam'

# A sample logging configuration. The only tangible logging
# performed by this configuration is to send an email to
# the site admins on every HTTP 500 error when DEBUG=False.
# See http://docs.djangoproject.com/en/dev/topics/logging for
# more details on how to customize your logging configuration.
#LOGGING = {
    #'version': 1,
    #'disable_existing_loggers': False,
    #'filters': {
        #'require_debug_false': {
            #'()': 'django.utils.log.RequireDebugFalse'
        #}
    #},
    #'handlers': {
        #'mail_admins': {
            #'level': 'ERROR',
            #'filters': ['require_debug_false'],
            #'class': 'django.utils.log.AdminEmailHandler'
        #}
    #},
    #'loggers': {
        #'django.request': {
            #'handlers': ['mail_admins'],
            #'level': 'ERROR',
            #'propagate': True,
        #},
    #}
#}


# override global settings (file in project root folder)
import djcelery
djcelery.setup_loader()

from django_yauth.settings import *
YAUTH_TYPE = 'intranet'
YAUSER_ADMIN_LOGIN = True
CREATE_USER_ON_ACCESS = False
CREATE_PROFILE_ON_ACCESS = False
REFRESH_SESSION_WITH_COOKIESET = True
YAUSER_PROFILES = (
    'core.User',
)

# ANG_FRONT = 'http://ang2.dumdum.yandex-team.ru'
ANG_FRONT = 'http://ang-back.yandex-team.ru'

try:
    from ui.env.settings import *
except ImportError:
    pass
