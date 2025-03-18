# coding: utf-8
from .base import AtBaseRepository

from ids.registry import registry
from ids.storages.null import NullStorage


class ClubRepository(AtBaseRepository):
    """
    Репозиторий "Клуб"

    """
    SERVICE = 'at'
    RESOURCES = 'club'

    def search(self, lookup):
        """
        lookup = {'uid': 123}
        Данные клуба

        lookup = {'owner': 321}
        Данные клубов пользователя 321
        """
        if 'owner' in lookup:
            uid = lookup['owner']
            data = self.connector.get('person_clubs', url_vars={'uid': uid})

            for club in data.get('clubs', []):
                yield club

        else:
            uid = lookup['uid']
            yield self.connector.get('club_info', url_vars={'uid': uid})


def factory(**options):
    storage = NullStorage()
    repository = ClubRepository(storage, **options)
    return repository


registry.add_repository(
    ClubRepository.SERVICE,
    ClubRepository.RESOURCES,
    factory,
)
