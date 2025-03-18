# -*- coding: utf-8 -*-

from ids.repositories.bound_base import BoundRepositoryBase

from ..connector import StartrekConnector


class StartrekBaseBoundRepository(BoundRepositoryBase):
    SERVICE = 'startrek'

    STARTREK_CONNECTOR_CLASS = StartrekConnector

    OPTIONS_KEYS_TO_COPY = ('server', 'oauth2_access_token', 'debug', '__uid',
                            '__login')

    def __init__(self, parent_resource, **options):
        super(StartrekBaseBoundRepository, self).__init__(
            parent_resource, **options)
        self.startrek = self.STARTREK_CONNECTOR_CLASS(self.options)
