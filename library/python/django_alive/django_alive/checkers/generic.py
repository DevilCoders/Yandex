# coding: utf-8
from __future__ import unicode_literals, absolute_import

from .base import Checker


class GenericChecker(Checker):
    def check(self, group):
        return True
