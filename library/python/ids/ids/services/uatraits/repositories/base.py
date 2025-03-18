# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.repositories.base import RepositoryBase

from ..connector import UatraitsConnector


class UatraitsRepository(RepositoryBase):
    def __init__(self, storage, **options):
        super(UatraitsRepository, self).__init__(storage, **options)
        self.connector = UatraitsConnector(**options)

    def post(self, lookup, data, **options):
        return self.connector.post(resource=self.RESOURCES, params=lookup, data=data, **options)
