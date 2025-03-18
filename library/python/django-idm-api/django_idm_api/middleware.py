# coding: utf-8

from __future__ import unicode_literals

import logging
import tvm2

from django.conf import settings
from django.core import exceptions
from django.utils.functional import cached_property

from django_idm_api import constants
from django_idm_api import conf

try:
    from django.utils.deprecation import MiddlewareMixin as MIDDLEWARE_BASE
except ImportError:
    MIDDLEWARE_BASE = object

log = logging.getLogger(__name__)


class CertificateMiddleware(MIDDLEWARE_BASE):
    """
    Валидируем клиентские сертификаты, где это необходимо (старая схема)
    https://doc.qloud.yandex-team.ru/doc/domain
    """

    def _check_certificate(self, request):
        ssl_verified = request.META.get('HTTP_X_QLOUD_SSL_VERIFIED', '')
        ssl_subject = request.META.get('HTTP_X_QLOUD_SSL_SUBJECT')
        ssl_issuer = request.META.get('HTTP_X_QLOUD_SSL_ISSUER')

        if not (
            ssl_verified.lower() in ['true', 'success'] and
            ssl_subject in constants.IDM_CERT_SUBJECTS and
            ssl_issuer in constants.IDM_CERT_ISSUERS
        ):
            log.error(
                'Invalid IDM certificate: verified=%r, subject=%r, issuer=%r; '
                'expected: verified in (\"true\", \"success\"), subject in %r, issuer in %r',
                ssl_verified, ssl_subject, ssl_issuer, constants.IDM_CERT_SUBJECTS, constants.IDM_CERT_ISSUERS
            )
            return False

        return True

    def process_request(self, request):
        if not request.path.startswith('/' + conf.IDM_URL_PREFIX):
            return

        if not self._check_certificate(request):
            raise exceptions.PermissionDenied('Invalid IDM client certificate')


class TVMMiddleware(MIDDLEWARE_BASE):
    """Валидируем TVM-тикеты (рекомендованная схема)"""

    @cached_property
    def tvm_client(self):
        tvm_params = settings.IDM_API_TVM_DEFAULTS
        tvm_params.update(conf.IDM_API_TVM_SETTINGS)
        return tvm2.TVM2(**tvm_params)

    def _check_tvm_ticket(self, request):
        tvm_ticket = request.META.get('HTTP_X_YA_SERVICE_TICKET')
        if not tvm_ticket:
            return False

        parsed_ticket = self.tvm_client.parse_service_ticket(tvm_ticket)
        if not parsed_ticket:
            return False

        return parsed_ticket.src == settings.IDM_TVM_CLIENT_ID

    def process_request(self, request):
        if not request.path.startswith('/' + conf.IDM_URL_PREFIX):
            return

        if not self._check_tvm_ticket(request):
            raise exceptions.PermissionDenied('TVM auth failed')


class MigrateFromCertificateToTVMMiddleware(CertificateMiddleware, TVMMiddleware):
    """Валидируем как сертификаты, так и TVM-тикеты (промежуточный этап миграции)"""

    def process_request(self, request):
        if not request.path.startswith('/' + conf.IDM_URL_PREFIX):
            return

        if not (self._check_certificate(request) or self._check_tvm_ticket(request)):
            raise exceptions.PermissionDenied(
                'Both IDM client certificate and TVM service ticket are missing or invalid',
            )
