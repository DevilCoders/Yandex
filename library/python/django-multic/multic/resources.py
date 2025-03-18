# coding: utf-8

import six

from django.db.models import Q
from django.db import models

from .filters import build_filter_cls


class ResourceRegistrator(type):
    """Метакласс для регистрации ресурсов - наследников SearchResourse"""
    def __init__(cls, name, bases, attrs):
        super(ResourceRegistrator, cls).__init__(name, bases, attrs)

        if hasattr(cls, 'name'):
            SearchResource.register[cls.name] = cls


class SearchResource(six.with_metaclass(ResourceRegistrator, object)):
    """Базовый класс для ресурсов, по которым можно поискать.

    В `register` метакласс складывает ссылки на наследников класса.
    `queryset` -- базовый кверисет, ограничивающий сущности, описываемы ресурсом
    `required_fields` -- минимальный набор полей для рендеринга в мультикомплите
    `extra_fields` -- дополнительные поля, которые можно запросить у ресурса
    помимо полей модели. Для добавления этих полей должны быть объявлены методы
    с названием extra_<имяполя>
    `search_conditions` -- лукапы для полей, по которым ищем. Если это список,
    то считается, что все поля строковые, можно также задавать словарь с
    ключами-функциями, которые будут использованы для приведения поисковой фразы
    к нужному типу
    `extra_methods_order` -- список имен экстра-полей, если важен порядок вызова
    их методов.
    Можно установить `filter_class` (форма из django-filter) для наследников
    класса.
    """

    register = {}
    queryset = required_fields = extra_fields = search_conditions = None
    extra_methods_order = []
    default_id_field = 'id'
    filter_fields = None

    def __init__(
            self, q, limit=None, requested_fields=None,
            id_field=None, filters=None, request=None):
        self.q = q
        self.limit = limit
        self.requested_fields = requested_fields or []
        self.id_field = id_field
        self.filters = filters
        self.request = request

    def get_values_iterator(self):
        """Получить итератор для выдачи отфильтрованных данных ресурса с
        запрошенными полями.
           1. Отфильтровать (`search`)
           2. Получить итератор, выдающий данные с запрошенными полями
        """
        queryset = self.apply_filters()
        filtered_queryset = self.search(queryset=queryset).distinct()
        return self.get_values(queryset=filtered_queryset)

    def apply_filters(self):
        queryset = self.queryset
        if self.filters:
            queryset = self.filter_cls(self.filters, queryset).qs
        return queryset

    @property
    def filter_cls(self):
        model = self.queryset.model
        filter_fields = self.filter_fields
        return build_filter_cls(model=model, filter_fields=filter_fields)

    def search(self, queryset):
        """Фильтрация по поисковой строке."""
        full_search_query = Q()
        for word in self.q.split():
            full_search_query &= self.search_word(word)

        if full_search_query:
            return queryset.filter(full_search_query)
        else:
            return queryset.none()

    def search_word(self, q):
        """Фильтрация ресурса по поисковому запросу из одного слова."""
        str_func = str if six.PY3 else unicode

        if isinstance(self.search_conditions, (list, tuple)):
            search_conditions = {str_func: self.search_conditions}
        else:
            search_conditions = dict(self.search_conditions)

        search_query = Q()
        for func, search_conditions in search_conditions.items():
            try:
                coerced = func(q)
            except ValueError:
                continue
            else:
                subquery = Q()
                for condition in search_conditions:
                    lookup = {condition: coerced}
                    subquery |= Q(**lookup)
                search_query |= subquery
        return search_query

    def get_queryset_values(self, queryset):
        return queryset.values(*self.fields)[:self.limit]

    def get_values(self, queryset):
        """Генератор сущностей ресурса. Добавляет extra-данные по запросу,
        удаляет лишние поля после обработки.
        """
        self.build_fields_list()
        for obj in self.get_queryset_values(queryset):
            obj = self.append_extra_data(obj)
            obj = self.pop_redundant_fields(obj)
            obj = self.post_rename_fields(obj)
            yield obj

    def build_fields_list(self):
        # дополним запрошенные поля обязательными
        self.all_requested_fields = set(self.requested_fields)
        self.all_requested_fields.update(self.required_fields)

        # поля для запроса к модели
        self.fields = set(self.all_requested_fields)
        if self.id_field:
            self.fields.add(self.id_field)
            # дополнительные поля
        self.extra_fields = self.extra_fields or {}
        self.requested_extra = list(set(self.fields) & set(self.extra_fields))

        # всегда выдаем строковое представление и id
        self.requested_extra.extend(['_text', '_id'])

        # выкидываем экстра-поля из fields
        self.fields = list(set(self.fields) - set(self.requested_extra))

        # для extra-данных могут понадобиться дополнительные поля из модели
        self.sort_extra_fields_using_order()
        for extra_field in self.requested_extra:
            required = self.extra_fields[extra_field].get('required_fields', [])
            self.fields.extend(required)

    def sort_extra_fields_using_order(self):
        """Если наследник указал порядок, в соответствии с которым нужно вызывать
        экстра-методы, то сортируем в соответствии с ним
        """
        order = getattr(self, 'extra_methods_order')
        if not order:
            return

        sorter = lambda field: order.index(field)
        self.requested_extra = sorted(self.requested_extra, key=sorter)

    def pop_redundant_fields(self, obj):
        """Выкинуть все поля, которые не были запрошены или предложены по
        умолчанию"""
        redundant_fields = set(self.fields) - set(self.all_requested_fields)
        for field in redundant_fields:
            del obj[field]
        return obj

    def post_rename_fields(self, obj):
        renames = getattr(self, 'fields_post_rename', None)
        if renames is None:
            return obj

        for field in obj:
            if field in renames:
                new_field_name = renames[field]
                obj[new_field_name] = obj[field]
                del obj[field]
        return obj

    # работа с extra
    def append_extra_data(self, obj):
        """Для каждого extra-поля ищет соответствующий метод с префиксом
        `extra_`, который добавит в него дополнительные данные.
        """
        for extra_field in self.requested_extra:
            extra_method = getattr(self, 'extra_' + extra_field, None)
            if extra_method:
                obj[extra_field] = extra_method(obj)
        return obj

    def extra__text(self, obj):
        """Добавляет строковое представление объекта в поле _text"""
        return unicode(obj)

    def extra__id(self, obj):
        """Добавляет уникальный идентификатор в поле _id"""
        return obj[self.id_field or self.default_id_field]

    def extra_ancestors(self, obj):
        """Метод получения родительских объектов из mptt."""
        lft, rght, tree_id = obj['lft'], obj['rght'], obj['tree_id']
        ancestors = self.model.objects.filter(
            tree_id=tree_id,
            rght__gt=rght,
            lft__lt=lft
        ).order_by('lft')
        fields_for_fetch = self.extra_fields['ancestors']['fields']
        return list(ancestors.values(*fields_for_fetch))

    # utils
    @classmethod
    def get_available_fields(cls):
        for field in cls.queryset.model._meta.fields:
            if isinstance(field, models.ForeignKey):
                yield field.name + '_id'
            else:
                yield field.name

        if cls.extra_fields:
            for field in cls.extra_fields.keys():
                yield field

    @property
    def model(self):
        return self.queryset.model
