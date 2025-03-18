# coding: utf-8
from __future__ import unicode_literals

from ids.lib.static_api import StaticApiGeneratedRepository

from ..connector import StaffConnector


class StaffRepository(StaticApiGeneratedRepository):
    connector_cls = StaffConnector
