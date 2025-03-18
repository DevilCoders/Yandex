# coding: utf-8
from __future__ import unicode_literals, absolute_import

from django_replicated.dbchecker import db_is_alive

from .base import Checker


class DBChecker(Checker):

    def __init__(self, name, stamper, alias=None):
        super(DBChecker, self).__init__(name, stamper)

        self.alias = alias or 'default'

    def check(self, group):
        is_alive = db_is_alive(self.alias)

        if is_alive:
            return True
