# coding: utf-8

from __future__ import unicode_literals

import six
import logging
import blackbox

if six.PY2:
    from urlparse import urlparse
elif six.PY3:
    from urllib.parse import urlparse

from django.conf import settings
from django.contrib.auth.models import AnonymousUser
from django.core.exceptions import PermissionDenied

from tvm2 import TVM2

from django_yauth.util import get_real_ip
from ..base import BaseMechanism
from .request import TvmImpersonatedRequest, TvmServiceRequest, TvmRequest

log = logging.getLogger(__name__)


class Mechanism(BaseMechanism):
    YandexUser = TvmImpersonatedRequest
    YandexServiceUser = TvmServiceRequest

    user_type = settings.YAUTH_TVM2_USER_HEADER
    service_type = settings.YAUTH_TVM2_SERVICE_HEADER

    def __init__(self):
        # При первом вызове получаем ключи и создаем контекст
        self.tvm = TVM2(
            client_id=settings.YAUTH_TVM2_CLIENT_ID,
            secret=settings.YAUTH_TVM2_SECRET,
            blackbox_client=self.blackbox,
            allowed_clients=self.allowed_clients,
            disk_cache_dir=settings.YAUTH_TVM2_DISK_CACHE_DIR,
            fetch_roles_for_idm_system=settings.YAUTH_TVM2_FETCH_ROLES_FOR_IDM_SYSTEM,
            tirole_env=settings.YAUTH_TVM2_TIROLE_ENV,
            tirole_check_src=settings.YAUTH_TVM2_TIROLE_CHECK_SRC,
            tirole_check_uid=settings.YAUTH_TVM2_TIROLE_CHECK_UID,
        )

    @property
    def blackbox(self):
        blackbox_host = urlparse(blackbox.BLACKBOX_URL).netloc
        return getattr(settings, 'YAUTH_TVM2_BLACKBOX_CLIENT') or settings.YAUTH_TVM2_BLACKBOX_MAP[blackbox_host]

    @property
    def allowed_clients(self):
        return settings.YAUTH_TVM2_ALLOWED_CLIENT_IDS

    @property
    def client_state(self):
        """
        Возвращает статус tvmauth клиента
        https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvmauth/tvmauth/__init__.py?rev=r8112549#L61
        """
        context = getattr(self.tvm, '_context', None)
        if context is not None:
            return context.status

    def extract_params(self, request):
        service_ticket = request.META.get(self.service_type)
        user_ticket = request.META.get(self.user_type)
        if user_ticket and not service_ticket:
            log.warning('User ticket present, service ticket expected but not passed')
        if not service_ticket:
            return

        return {'user_ticket': user_ticket,
                'service_ticket': service_ticket,
                'user_ip': get_real_ip(request),
                }

    def has_perm(self, user_obj, perm, obj=None):
        """
        Для совместимости с аутентификационными бекэндами джанго.

        @param perm: oauth scope
        """
        if isinstance(user_obj, (self.AnonymousYandexUser, AnonymousUser)):
            raise PermissionDenied('no anonymous access')

        if isinstance(user_obj, TvmRequest):
            result = user_obj.check_scopes(perm)
            if not result:
                raise PermissionDenied('allowed scopes are "%s"' % ','.join(user_obj.authenticated_by.scopes))
            return True
        return False

    def apply(self, service_ticket, user_ip, user_ticket=None):
        parsed_user_ticket = None
        parsed_service_ticket = self.tvm.parse_service_ticket(service_ticket)
        if user_ticket is not None:
            parsed_user_ticket = self.tvm.parse_user_ticket(user_ticket)

        if not parsed_service_ticket:
            log.warning('Request with not valid service ticket was made')
        elif not parsed_user_ticket and user_ticket is not None:
            log.warning('Request with not valid user ticket was made')
        else:
            if parsed_user_ticket:
                return self.get_person_user(
                    user_ticket=parsed_user_ticket,
                    service_ticket=parsed_service_ticket,
                    raw_user_ticket=user_ticket,
                    raw_service_ticket=service_ticket,
                    user_ip=user_ip,
                )
            else:
                return self.get_service_user(
                    service_ticket=parsed_service_ticket,
                    raw_service_ticket=service_ticket,
                    user_ip=user_ip,
                )

        return self.anonymous()

    def get_person_user(self, user_ticket, service_ticket, raw_user_ticket, raw_service_ticket, user_ip):
        default_uid = user_ticket.default_uid
        return self.YandexUser(
            user_ticket=user_ticket,
            service_ticket=service_ticket,
            uid=default_uid, mechanism=self,
            raw_user_ticket=raw_user_ticket,
            raw_service_ticket=raw_service_ticket,
            user_ip=user_ip,
        )

    def get_service_user(self, service_ticket, raw_service_ticket, user_ip):
        return self.YandexServiceUser(
            service_ticket=service_ticket,
            uid=None, mechanism=self,
            raw_service_ticket=raw_service_ticket,
            user_ip=user_ip,
        )

    def anonymous(self):
        return self.AnonymousYandexUser(mechanism=self)
