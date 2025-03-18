# -*- coding: utf-8 -*-

from ids.repositories.base import RepositoryBase

from ..connector import StartrekConnector


class StartrekBaseRepository(RepositoryBase):
    SERVICE = 'startrek'

    STARTREK_CONNECTOR_CLASS = StartrekConnector

    def __init__(self, storage=None, **options):
        super(StartrekBaseRepository, self).__init__(storage=storage, **options)
        self.startrek = self.STARTREK_CONNECTOR_CLASS(self.options)

    def _get_default_options(self):
        """
        oauth2_access_token(str): авторизационный токен для oauth2
        """

        d = super(StartrekBaseRepository, self)._get_default_options()

        d.update({
            'oauth2_access_token': None,
            'debug': False
        })

        return d

    def get_user_session_id(self):
        if '__uid' in self.options:
            return self.options['__uid']
        return self.options['oauth2_access_token']
