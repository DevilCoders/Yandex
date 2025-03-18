# coding: utf-8
from __future__ import unicode_literals, absolute_import
import six
from django.conf import settings

from .utils import import_object


class ConfigCache(object):

    def __init__(self, conf=None):
        self.conf = {'stampers': {}, 'checks': {}, 'groups': {}}
        self.conf.update(conf or settings.ALIVE_CONF)

        self.stampers = None
        self.checks = None

    def get_groups(self):
        return self.conf['groups']

    def get_stamper(self, stamper):
        self._init()

        return self.stampers[stamper]

    def get_check(self, check):
        self._init()

        return self.checks[check]

    def get_checks(self, checks):
        return dict((check, self.get_check(check)) for check in checks)

    def get_all_checks(self):
        self._init()

        return dict((check, self.get_check(check)) for check in self.checks)

    def get_group_checks(self, group):
        return self.get_checks(self.conf['groups'][group])

    def _init(self):
        if self.stampers is not None or self.checks is not None:
            return

        self.stampers = {}

        for name, conf in six.iteritems(self.conf['stampers']):
            conf = dict(conf)
            cls = self._get_class_by_path(conf.pop('class'))

            self.stampers[name] = cls(**conf)

        self.checks = {}

        for name, conf in six.iteritems(self.conf['checks']):
            conf = dict(conf)
            cls = self._get_class_by_path(conf.pop('class'))
            stamper = self.stampers[conf.pop('stamper')]

            self.checks[name] = cls(name=name, stamper=stamper, **conf)

    def _get_class_by_path(self, path):
        return import_object(path)


config_cache = ConfigCache()


def get_groups():
    return config_cache.get_groups()


def get_stamper(stamper):
    return config_cache.get_stamper(stamper)


def get_check(check):
    return config_cache.get_check(check)


def get_checks(checks):
    return config_cache.get_checks(checks)


def get_all_checks():
    return config_cache.get_all_checks()


def get_group_checks(group):
    return config_cache.get_group_checks(group)


def get_alive_middleware_reduce():
    return import_object(settings.ALIVE_MIDDLEWARE_REDUCE)
