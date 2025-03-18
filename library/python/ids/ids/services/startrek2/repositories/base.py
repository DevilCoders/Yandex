# coding: utf-8
import six

from ids.repositories.base import RepositoryBase

from startrek_client import Startrek
from startrek_client.objects import Resource

from ..connector import StConnector


class StBaseRepository(RepositoryBase):
    """ Класс репозитория для apiv2 """
    SERVICE = 'startrek2'

    def __init__(self, storage, oauth_token=None, **options):
        super(StBaseRepository, self).__init__(storage, **options)
        if oauth_token is None:
            kwargs = options
        else:
            kwargs = options.copy()
            kwargs['oauth_token'] = oauth_token
        connector = StConnector(**kwargs)
        self.client = Startrek(connection=connector)

    def _common_lookup_hook(self, lookup):
        """
        Все лукапы передаются в стартрек-клиент "как есть".
        """
        return lookup

    def handle_result(self, raw):
        if isinstance(raw, Resource):
            # один объект
            raw = [raw]
        return iter(raw)

    def getiter_from_service(self, lookup):
        if isinstance(lookup, six.string_types):
            lookup = {'id': lookup}
        return self.handle_result(
            self.startrek_client_resourses.get_all(**lookup)
        )

    @property
    def startrek_client_resourses(self):
        return getattr(self.client, self.RESOURCES)

    def __getattr__(self, item):
        return getattr(self.startrek_client_resourses, item)

    # эти методы есть в этом классе, а нам нужны методы из стартрек-клиента
    def create(self, *args, **kwargs):
        return self.startrek_client_resourses.create(*args, **kwargs)

    def update(self, *args, **kwargs):
        return self.startrek_client_resourses.update(*args, **kwargs)

    def delete(self, *args, **kwargs):
        return self.startrek_client_resourses.delete(*args, **kwargs)


def create_repository(name):
    def factory(storage=None, **options):
        base_repo = StBaseRepository(storage, **options)
        base_repo.RESOURCES = name
        return base_repo

    return StBaseRepository.SERVICE, name, factory
