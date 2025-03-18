# -*- coding: utf-8 -*-

import hashlib
import itertools
from copy import copy

import six

from ids.exceptions import EmptyIteratorError
from ids.resource import Resource
from ids.utils.fields_mapping import unbeautify_fields
from ids.storages.null import NullStorage


class RepositoryBase(object):
    '''
    Является провайдером ресурсов (Resource) определенного типа.
    Конкретные наследники знают, как доставать нужные данные.
    '''

    SERVICE = None
    RESOURCES = None

    connector_cls = None

    def _get_default_options(self):
        """
        use_service(bool): использовать ли сервис для получения данных
        fields_mapping(dict): маппинг пользовательских имен полей на реальные
        """
        return {
            'use_service': True,
            'fields_mapping': {},
        }

    def __init__(self, storage=None, **options):
        """

        @param storage: объект хранилища
        @param options: настройки конкретного репозитория

        Инициализация и настройка репозитория.
        Возможные опции в _get_default_options.
        """

        super(RepositoryBase, self).__init__()

        # стэк состояний словаря опций, для использования временного
        # набора опций с with-оператором
        self._options_state_stack = []

        self.storage = storage or NullStorage()

        self.options = self._get_default_options()
        self.options.update(options)

        if self.connector_cls:
            self.connector = self.connector_cls(**options)

    @property
    def use_storage(self):
        return not isinstance(self.storage, NullStorage)

    @property
    def use_service(self):
        return self.options['use_service']

    def _wrap_to_resource(self, obj, resource=None):
        '''
        @param obj: object
        @param resource: Resource

        Связывает переданный ресурс с объектом.
        Если ресурс не указан, создается новый.
        '''

        if resource is None:
            resource = Resource()

        resource['__repository__'] = self
        resource['__raw__'] = obj

        return resource

    def _make_storage_key(self, lookup):
        lookup = self._common_lookup_hook(lookup)

        # lookp_hash == key1:val1-key2:val2-...
        lookup_hash = u'-'.join(u'{0}:{1}'.format(k, v)
                                for k, v in sorted(six.iteritems(lookup)))

        str_key = u'_'.join([
            self.SERVICE,
            self.RESOURCES,
            self.get_user_session_id(),
            lookup_hash,
        ])

        # hashlib незнает юникода
        if isinstance(str_key, six.text_type):
            str_key = str_key.encode('utf8')

        md5_key = hashlib.md5()
        md5_key.update(str_key)

        return md5_key.hexdigest()

    def get_user_session_id(self):
        return 'NOT-NEEDED'

    ###
    ### Region: repository methods
    ###
    def getiter_from_storage(self, lookup, storage_key=None):
        '''
        Шаблонный метод с реализацией в наследниках, вызываемый из getiter.
        Возвращает итератор на коллекцию ресурсов, взятых из хранилища.
        '''

        key = storage_key or self._make_storage_key(lookup)
        cached = self.storage.get(key)
        if cached is not None:
            for resource in cached:
                yield resource

    def getiter_from_service(self, lookup):
        '''
        Шаблонный метод с реализацией в наследниках, вызываемый из getiter.
        Возвращает итератор на коллекцию ресурсов, взятых с сервиса.
        '''

        raise NotImplementedError('class is abstract')

    def _common_lookup_hook(self, lookup):
        lookup = unbeautify_fields(lookup, self.options['fields_mapping'])
        return lookup

    def _getiter_lookup_hook(self, lookup):
        lookup = self._common_lookup_hook(lookup)
        return lookup

    def getiter(self, lookup, **options):
        """
        @param lookup: dict
        @returns: iter for list of Resources

        @example: obj.getiter({'id': 'PLAN-666'}, use_storage=False)

        Фильтрует ресурсы в соответствии с заданным lookup'ом и возвращает
        итератор на их список.
        Опции перезаписывают глобальные только на один раз.
        TODO: Если итератор не дойдет до конца последовательности, то результат
        не будет добавлен в хранилище.
        """
        with self._temp_options(options):
            lookup = self._getiter_lookup_hook(lookup)
            storage_key = self._make_storage_key(lookup)

            if self.options['use_service']:
                it = None
                if self.use_storage:
                    it = self.getiter_from_storage(lookup, storage_key=storage_key)

                try:
                    first = six.next(it)
                except (AttributeError, StopIteration, TypeError):  # it == None or empty
                    it = self.getiter_from_service(lookup)
                    if self.use_storage:
                        # осторожно, можно сильно нагрузить сервис-источник данных
                        # если объекты возвращаются кусками или объектов очень много
                        # не используйте если у вас очень много объектов
                        self.storage.add(
                            storage_key,
                            list(it)
                        )
                        it = self.getiter_from_storage(lookup, storage_key=storage_key)
                else:
                    it = itertools.chain(iter([first]), it)

                return it

            # на данный момент options['use_service'] == False
            it = iter([])
            if self.use_storage:
                it = self.getiter_from_storage(lookup, storage_key=storage_key)

            return it

    def get(self, lookup, **options):
        """
        Алиас для getiter с мгновенным результатом.
        Опции перезаписывают глобальные только на один раз.

        @returns: list of Resources
        """

        return list(self.getiter(lookup, **options))

    def get_one(self, lookup, **options):
        """
        Алиас для get с одним результатом.
        Опции перезаписывают глобальные только на один раз.

        @returns: Resource
        """
        for item in self.getiter(lookup):
            return item
        raise EmptyIteratorError()

    def update(self, resource, fields=None, **options):
        '''
        @param resource: Resource
        @param fields: dict

        @example: obj.update(resource, fields={'summary': 'new summary})

        Обновляет сущетвующий ресурс в репозитории.
        Опции перезаписывают глобальные только на один раз.
        '''

        if fields is None:
            fields = {}

        with self._temp_options(options):
            fields = self._common_lookup_hook(fields)
            self.update_(resource, fields)

    def update_(self, resource, fields):
        raise NotImplementedError('class is abstract')

    def create(self, fields=None, **options):
        '''
        @param fields: dict
        @returns Resource

        @example: obj.create(fields={'summary': 'new summary})

        Создает новый ресурс, соответствующий репозиторию.
        Опции перезаписывают глобальные только на один раз.
        '''

        if fields is None:
            fields = {}

        with self._temp_options(options):
            fields = self._common_lookup_hook(fields)
            return self.create_(fields)

    def create_(self, fields):
        raise NotImplementedError('class is abstract')

    def delete(self, resource, **options):
        '''
        @param resource: Resource

        @example: obj.delete(resource)

        Удаляет существующий ресурс из репозитория.
        Опции перезаписывают глобальные только на один раз.
        '''

        with self._temp_options(options):
            self.delete_(resource)

    def delete_(self, resource):
        raise NotImplementedError('class is abstract')

    ###
    ### Region: context manager protocol
    ###
    def _temp_options(self, additional_options=None):
        '''
        @param additional_options: dict

        @example: with _temp_options({'temp': value})

        Точка входа для протокола контекстного менеджера.
        Временно меняет опции объекта.
        '''

        if additional_options == None:
            additional_options = {}

        self._options_state_stack.append(copy(self.options))
        self.options.update(additional_options)

        return self

    def __enter__(self):
        # если with используется с temp_options
        if len(self._options_state_stack) > 0:
            pass

    def __exit__(self, *args):
        # если with используется с temp_options
        if len(self._options_state_stack) > 0:
            self.options = self._options_state_stack.pop()

        return False

    def __repr__(self):
        return '<{cls}: {identity}>'.format(
            cls=self.__class__.__name__,
            identity=self.SERVICE + '|' + self.RESOURCES,
        )
