# coding: utf-8
from __future__ import unicode_literals

import logging
from django.conf import settings

from django_yauth.user import YandexUser
from django_yauth.authentication_mechanisms.cookie import BlackBoxMixin

log = logging.getLogger(__name__)


class TvmRequest(YandexUser):
    is_impersonated = None

    @property
    def scopes(self):
        return []

    def check_scopes(self, *required_scopes):
        """
        Проверить что пользователь удовлетворяет скоупам.
        В продакшен можно делать так:
        if yauser.authenticated_by.mechanism_name == 'tvm':
            if yauser.check_scopes('fly_japan', 'bow_to_statue'):
                start_sacred_journey(yauser, 'hidden temple')
            else:
                sorry_bro(yauser)

        @rtype: bool
        """
        return set(required_scopes).issubset(self.scopes)


class TvmServiceRequest(TvmRequest):
    """ Пришел сервис"""
    is_impersonated = False


class TvmImpersonatedRequest(BlackBoxMixin, TvmServiceRequest):
    """ Пришел пользователь"""
    is_impersonated = True

    def __init__(self, *args, **kwargs):
        super(TvmImpersonatedRequest, self).__init__(*args, **kwargs)
        if settings.YAUTH_TVM2_GET_USER_INFO:
            self._populate_with_user_data()

    @property
    def uids(self):
        return self.user_ticket.uids

    @property
    def scopes(self):
        return self.user_ticket.scopes

    def _populate_with_user_data(self):
        log.info('Populating yauser with data from blackbox')

        if settings.YAUTH_TVM2_USE_USER_TICKET_FOR_USER_INFO and hasattr(self.blackbox, 'user_ticket'):
            authinfo = self.blackbox.user_ticket(
                user_ticket=self.raw_user_ticket,
                dbfields=self.bb_fields,
                **self.bb_params
            )
        else:
            authinfo = self.blackbox.userinfo(
                self.uid,
                self.user_ip or '127.0.0.1',
                dbfields=self.bb_fields,
                **self.bb_params
            )
        authinfo.url = self.blackbox.url

        if hasattr(authinfo, 'attributes'):
            self.attributes = authinfo.attributes

        self.fields = authinfo.fields
        self.emails = authinfo.emails
        self.default_email = authinfo.default_email
        self.blackbox_result = authinfo

        log.info('Successfully populate yauser with data from blackbox')
