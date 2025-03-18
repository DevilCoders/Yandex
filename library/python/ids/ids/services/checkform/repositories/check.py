# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import re

from ids.exceptions import BackendError
from ids.registry import registry
from ids.repositories.base import RepositoryBase

from ..connector import CheckFormConnector


@registry.add_simple
class CheckFormRepository(RepositoryBase):
    SERVICE = 'checkform'
    RESOURCES = 'check'

    def __init__(self, storage, **options):
        super(CheckFormRepository, self).__init__(storage, **options)
        self.connector = CheckFormConnector(**options)

    def get(self, service, ip, host, realpath=None, form_name=None, email=None,
            subject=None, comment=None, uid=None, login=None, **options):
        return self._make_request(
            'get', service, ip, host, realpath=realpath, form_name=form_name, email=email,
            subject=subject, comment=comment, uid=uid, login=login, **options
        )

    def post(self, service, ip, host, realpath=None, form_name=None, email=None,
             subject=None, comment=None, uid=None, login=None, **options):
        return self._make_request(
            'post', service, ip, host, realpath=realpath, form_name=form_name, email=email,
            subject=subject, comment=comment, uid=uid, login=login, **options
        )

    def _make_request(self, method, service, ip, host, realpath, form_name, email,
                      subject, comment, uid, login, **options):
        data = {
            key: value
            for key, value in (
                ('so_service', service),
                ('so_ip', ip),
                ('so_host', host),
                ('so_realpath', realpath),
                ('so_form_name', form_name),
                ('so_email', email),
                ('so_subject', subject),
                ('so_comment', comment),
                ('so_uid', uid),
                ('so_login', login),
            )
            if value is not None
        }
        if method == 'get':
            options['params'] = data
        else:
            options['data'] = data
        response = getattr(self.connector, method)(resource=self.RESOURCES, **options)
        m = re.search(r'\<spam\>([01])\</spam\>', response.text)
        if m:
            return m.group(1) == '1'
        raise BackendError('unexpected response', response=response)
