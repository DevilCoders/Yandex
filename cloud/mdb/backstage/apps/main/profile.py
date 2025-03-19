import logging
from django.conf import settings

import cloud.mdb.backstage.apps.main.models as models


logger = logging.getLogger('backstage.main.profile')

DEFAULT_PROFILE_SETTINGS = {
    'theme': 'light',
    'timezone': settings.CONFIG.get('profile_default_tz', 'Europe/Moscow'),
}


def get_profile_model_instance(iam_user):
    profile, _ = models.UserProfile.objects.get_or_create(
        sub=iam_user.sub,
        defaults={
            'sub': iam_user.sub,
            'settings': DEFAULT_PROFILE_SETTINGS
        }
    )
    return profile


class ProfileDescriptor:
    def __get__(self, iam_user, owner):
        if not hasattr(iam_user, '_profile'):
            iam_user._profile = self._get_profile(iam_user)
        return iam_user._profile

    def _get_profile(self, iam_user):
        return Profile(iam_user)


class Profile:
    def __init__(self, iam_user):
        self._iam_user = iam_user
        self._settings_data = None

    def __getattr__(self, key):
        if key in self._settings:
            return self._settings[key]
        else:
            return object.__getattribute__(self, key)

    @property
    def _settings(self):
        if self._settings_data is None:
            self._settings_data = self.get_settings()
        return self._settings_data

    def get_settings(self):
        if settings.CONFIG.no_database:
            logger.debug('Database is disabled: setting default profile')
            return DEFAULT_PROFILE_SETTINGS
        if not self._iam_user.is_authenticated():
            logger.debug('User is not authenticated: setting default profile')
            return DEFAULT_PROFILE_SETTINGS
        try:
            profile = get_profile_model_instance(self._iam_user)
        except Exception as err:
            logger.exception(f'Failed to get user profile settings: {err}. Setting default profile')
            return DEFAULT_PROFILE_SETTINGS
        else:
            profile_settings = {}
            for key in DEFAULT_PROFILE_SETTINGS.keys():
                profile_settings[key] = profile.settings.get(key, DEFAULT_PROFILE_SETTINGS[key])
            return profile_settings
