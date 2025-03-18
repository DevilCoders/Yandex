# coding: utf-8

from __future__ import unicode_literals

import logging

from django_yauth.util import get_real_ip, get_current_host
from django_yauth.user import AnonymousYandexUser


log = logging.getLogger(__name__)


class BaseMechanism(object):
    AnonymousYandexUser = AnonymousYandexUser

    def is_applicable(self, request):
        return self.extract_params(request)

    def apply(self, **kwargs):
        raise NotImplementedError

    def authenticate(self, request):
        """
        Совместимость с джанговой ауентентификацией.

        Вы можете подключать эти бэкэнды через AUTHENTICATION_BACKENDS.
        Однако нужно помнить, что в ответ на django.contrib.auth.authenticate()
        вам вернется не инстанс AUTH_USER_MODEL, а инстанс
        django_yauth.user.YandexUser.

        Тем не менее у YandexUser тоже есть has_perm и вы можете
        использовать его "как будто у вас почти AUTH_USER_MODEL":
        вызывать has_perm, has_perms.

        @rtype: YandexUser | None
        """
        params = self.is_applicable(request)
        if params is not None:
            user = self.apply(**params)
            if isinstance(user, self.AnonymousYandexUser):
                # для совместимости с django нужно вернуть None
                return None
            return user
        return None

    def extract_params(self, request):
        """
        Вернуть None, если аутентифицировать невозможно или словарь параметров для self.apply

        @rtype: dict | None
        """
        raise NotImplementedError

    @property
    def mechanism_name(self):
        return self.__module__.split('.')[-1]

    def __repr__(self):
        return 'yauth mechanism "%s"' % self.mechanism_name

    @classmethod
    def get_user_ip(self, request):
        return get_real_ip(request)

    @classmethod
    def get_current_host(self, request):
        return get_current_host(request)
