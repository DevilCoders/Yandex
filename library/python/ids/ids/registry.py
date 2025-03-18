# -*- coding: utf-8 -*-
import sys

from ids.storages.null import NullStorage
from ids.exceptions import (
    KeyAlreadyExistsError,
    KeyIsAbsentError,
)


def _make_key(service, resource_type):
    return service, resource_type


class _Registry(object):
    '''
    Управляет списком всех поддерживаемых репозиториев.
    Работает через единственный экземпляр.
    '''

    def __init__(self):
        super(_Registry, self).__init__()
        self.storage = {}

    def add_repository(self, service, resource_type, repository_factory):
        '''
        @param service: str
        @param resource_type: str

        @example obj.add_repository('jira', 'tickets', JiraTicketsRepository)

        Добавляет способ создания репозитория в список доступных.
        '''

        key = _make_key(service, resource_type)
        if key in self.storage:
            raise KeyAlreadyExistsError(
                'key({0}, {1}) is already exists in registry'
                .format(service, resource_type)
            )

        self.storage[key] = repository_factory

    def add_simple(self, repo_cls):
        def default_factory(**options):
            options['storage'] = options.get('storage') or NullStorage()
            repository = repo_cls(**options)
            return repository

        self.add_repository(
            service=repo_cls.SERVICE,
            resource_type=repo_cls.RESOURCES,
            repository_factory=default_factory,
        )
        return repo_cls

    def _import_repo_module(self, name):
        if name not in sys.modules:
            __import__(name)

        return sys.modules[name]

    def get_repository(self, service, resource_type, **options):
        '''
        @param service: str
        @param resource_type: str
        @param options: dict
        @returns Repository

        @example repository = obj.get_repository('jira', 'tickets')

        Возвращает экземпляр репозитория, соответствующий сервису service
        и типу ресурса resource_type, созданный заново фабрикой из хранилища.
        Для большей гибкости поддерживается передача произвольных опций в фабрику.
        '''

        key = _make_key(service, resource_type)

        if key not in self.storage:

            module_service_name = ('ids.services.{0}.repositories'.format(service))
            module_name = 'ids.services.{0}.repositories.{1}'.format(
                service, resource_type)

            # сначала попробуем импортировать модуль репозиториев сервиса
            try:
                self._import_repo_module(module_service_name)
            except ImportError:
                pass

            if key not in self.storage:
                # если не зарегистрировалось, импортируем конкретный
                # модуль репозитория
                try:
                    self._import_repo_module(module_name)
                except ImportError:
                    raise KeyIsAbsentError(
                        'there is no key({0}, {1}) in registry'
                        .format(service, resource_type)
                    )

        return self.storage[key](**options)

    def delete_repository(self, service, resource_type):
        '''
        @param service: str
        @param resource_type: str

        @example obj.delete_repository('jira', 'tickets')

        Удаляет фабрику репозитория из хранилища.
        '''

        key = _make_key(service, resource_type)
        if key not in self.storage:
            raise KeyIsAbsentError('no key({0}, {1}) in storage')

        self.storage.pop(key)

registry = _Registry()  # синглтон
