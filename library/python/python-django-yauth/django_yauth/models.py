import warnings

from django.conf import settings
from django.db.models import signals
try:
    from django.db.models import get_model
except ImportError:  # Django > 1.8
    from django.apps import apps
    get_model = apps.get_model

from django_yauth.user import YandexUser, ProfileDescriptor, ProfileUserDescriptor
from django_yauth.util import get_setting


def add_reference(sender, **kwargs):
    app_label, model_name = sender._meta.app_label, sender.__name__
    if '%s.%s' % (app_label, model_name) not in get_setting(['YAUSER_PROFILES', 'YAUTH_YAUSER_PROFILES']):
        return

    descriptor_name = ProfileDescriptor.name_for_model(sender)
    setattr(YandexUser, descriptor_name, ProfileDescriptor(sender))

    sender.yauser = ProfileUserDescriptor()


def get_profile_models():
    for profile_name in get_setting(['YAUSER_PROFILES', 'YAUTH_YAUSER_PROFILES']):
        app_label, model_name = profile_name.rsplit('.', 1)
        model = get_model(app_label, model_name)
        if model is None:
            continue

        yield model


def add_profile_refs():
    for model in get_profile_models():
        add_reference(model)


try:
    from django.apps import AppConfig
except ImportError:
    signals.class_prepared.connect(add_reference)
    add_profile_refs()
else:
    class YauthConfig(AppConfig):
        name = 'django_yauth'

        def ready(self):
            signals.class_prepared.connect(add_reference)
            add_profile_refs()
    default_app_config = 'django_yauth.models.YauthConfig'


DEPRICATED_SETTINGS = [('YAUTH_USE_NATIVE_USER', 'YAUSER_ADMIN_LOGIN'),
                       ('YAUTH_CREATE_PROFILE_ON_ACCESS', 'CREATE_PROFILE_ON_ACCESS'),
                       ('YAUTH_CREATE_USER_ON_ACCESS', 'CREATE_USER_ON_ACCESS'),
                       ('YAUTH_PASSPORT_FIELDS', 'PASSPORT_FIELDS'),
                       ('YAUTH_YAUSER_PROFILES', 'YAUSER_PROFILES'),
                       ('YAUTH_PASSPORT_SERVICE_SLUG', 'PASSPORT_SERVICE_SLUG'),
                       ('YAUTH_REFRESH_SESSION', 'REFRESH_SESSION'),
                       ('YAUTH_COOKIE_CHECK_COOKIE_NAME', 'COOKIE_CHECK_COOKIE_NAME')]

for new_name, old_name in DEPRICATED_SETTINGS:
    if hasattr(settings, old_name):
        warnings.warn('`%s` is depricated. Use `%s` instead.' % (old_name, new_name),
                      DeprecationWarning)
