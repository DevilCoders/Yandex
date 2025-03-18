# coding: utf-8
from .base import AtBaseRepository

from ids.registry import registry
from ids.storages.null import NullStorage


class PeopleRepository(AtBaseRepository):
    """
    Репозиторий "Пользователь"

    """
    SERVICE = 'at'
    RESOURCES = 'user'

    def search(self, lookup):
        """
        lookup = {'uid': 123}
        Данные пользователя
        """
        uid = lookup['uid']
        return [self.connector.get('person_info', url_vars={'uid': uid})]


def factory(**options):
    storage = NullStorage()
    repository = PeopleRepository(storage, **options)
    return repository


registry.add_repository(
    PeopleRepository.SERVICE,
    PeopleRepository.RESOURCES,
    factory,
)
