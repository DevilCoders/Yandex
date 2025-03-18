# coding: utf-8

import datetime
import logging
import os

from django.conf import settings
from django.utils import timezone

from ..utils import current_host, import_object
from ..state import CheckState


log = logging.getLogger(__name__)


class Checker(object):

    def __init__(self, name, stamper):
        self.name = name
        self.stamper = stamper

    def check(self, group):
        raise NotImplementedError

    def do_check(self, group):
        try:
            result = self.check(group)
        except Exception:
            log.exception('Check `%s` on %s[%s] failed:', self.name, self.get_current_host(), group)
        else:
            if result:
                data = result if not isinstance(result, bool) else None

                self.do_stamp(group, data)

    def do_stamp(self, group, data=None, host=None):
        current_host = host or self.get_current_host()
        try:
            self.stamper.set(self.name, current_host, group, data)
        except Exception:
            log.exception(
                'Stamp by %s for check `%s` on %s[%s] with data=%s failed:',
                self.stamper, self.name, current_host, group, data
            )

    def get_state(self, host=None, group=None):
        return dict(
            (s.host, CheckState(self, s))
            for s in self.stamper.get(self.name, host=host, group=group)
        )

    def get_state_self(self, group=None):
        return [CheckState(self, s) for s in self.stamper.get(self.name, self.get_current_host(), group)]

    def get_current_host(self):
        return current_host

    def get_stamp_status(self, stamp):
        timeout = datetime.timedelta(seconds=settings.ALIVE_CHECK_TIMEOUT)

        return stamp.timestamp + timeout >= timezone.now()


class LazyClientChecker(Checker):
    def __init__(self, name, stamper, client_path):
        self.client_path = client_path

        self._client = None

        super(LazyClientChecker, self).__init__(name, stamper)

    @property
    def client(self):
        if self._client is None:
            client = import_object(self.client_path)

            if callable(client):
                client = client()

            self._client = client

        return self._client


class BaseStorageChecker(LazyClientChecker):
    key_separator = '-'

    def __init__(self, name, stamper, client_path, prefix='alive-checker'):
        super(BaseStorageChecker, self).__init__(name, stamper, client_path)

        self.prefix = prefix

    def check(self, group):
        key = self.create_key(group)
        original_value = self.create_value()

        self.put(key, original_value)

        value = self.pick(key)

        if value == original_value:
            return True

    def create_key(self, group):
        return self.key_separator.join([self.prefix, self.name, self.get_current_host(), group])

    def create_value(self):
        return '%s@%s' % (os.getpid(), timezone.now())

    def put(self, key, value):
        raise NotImplementedError

    def pick(self, key):
        raise NotImplementedError
