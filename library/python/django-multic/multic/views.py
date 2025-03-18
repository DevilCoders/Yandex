# -*- coding: utf-8 -*-
import simplejson
from collections import OrderedDict
from itertools import cycle, islice

from django.http import HttpResponse
from django.views.generic.base import View
from django.utils.decorators import method_decorator
from django.contrib.auth.decorators import login_required

from .multic_json import CustomJSONEncoder
from .forms import MulticompleteParamsValidator
from .resources import SearchResource

from . import DoesNotExistResources
if not SearchResource.register:
    raise DoesNotExistResources


# TODO: check refferer decorator
class MulticompleteView(View):
    DEFAULT_LIMIT = 8
    DEFAULT_FORMAT = 'jsonp'
    FORMATS = ('jsonp', 'json')

    _search_resources = SearchResource.register
    params_validator = MulticompleteParamsValidator

    @method_decorator(login_required)
    def dispatch(self, *args, **kwargs):
        return super(MulticompleteView, self).dispatch(*args, **kwargs)

    def get(self, request, *args, **kwargs):
        """Последовательность шагов в формировании выдачи:
           1. Валидация (`params_validator`)
           2. Если данные валидны, формируем данные (`prepare_data`)
           3. Сериализуем данные или ошибки (`serialize`)
           4. Формируем HttpResponse (`build_response`)
        """
        validator = self.params_validator(request.GET)

        if validator.errors:
            data = {'errors': validator.errors}
            status_code = 418
        else:
            self.params = validator.cleaned_data
            self.params['limit'] = self.params['limit'] or self.DEFAULT_LIMIT
            self.params['filters'] = self.fetch_filters(request.GET)
            data = self.prepare_data()
            status_code = 200

        serialized = self.serialize(data)
        return self.build_response(serialized, status_code=status_code)

    def fetch_filters(self, get_params):
        """
        Достать из запроса параметры, которые относятся к фильтрации.
        Они имеют вид <resource_type>__<filter_name>=<filter_value>
        """
        filters = {}
        for key, value in get_params.items():
            if '__' not in key:
                continue
            resource_type, filter_name = key.split('__', 1)
            resource_filters = filters.setdefault(resource_type, {})
            resource_filters[filter_name] = value
        return filters

    # формирование данных
    def prepare_data(self):
        """Единственная задача метода - получить данные для выдачи и вернуть их
        в виде списка объектов built-in типов.
        """
        iterators_dict = self.build_iterators()
        data_iter = islice(self.fetch_data(iterators_dict),
                           self.params['limit'])
        data = list(data_iter)
        data = self.sort(data)
        return data

    def build_iterators(self):
        """Получить из ресурсов итераторы с данными, отфильтрованными по
        поисковой фразе и с запрошенными полями.

        @return dict {resource_type: resource_iterator}
        """
        resource_iterators = OrderedDict()
        for type_ in self.params['types']:
            resourse_cls = self._search_resources[type_]

            fields = self.params[type_ + '_fields']
            id_field = self.params[type_ + '_id']
            resource = resourse_cls(
                q=self.params['q'],
                id_field=id_field,
                limit=self.params['limit'],
                requested_fields=fields,
                filters=self.params['filters'].get(type_),
                request=self.request,
            )

            resource_iterator = resource.get_values_iterator()
            resource_iterators[type_] = resource_iterator
        return resource_iterators

    def fetch_data(self, iterators_dict):
        """roundrobin - получаем данные по очереди из каждой последовательности
        ('ABC', 'D', 'EF') --> A D E B F C
        """
        while iterators_dict:
            to_delete = []
            for resource_type, iterator in iterators_dict.items():
                try:
                    obj = next(iterator)
                    obj['_type'] = resource_type
                    yield obj
                except StopIteration:
                    to_delete.append(resource_type)
            for resource_type in to_delete:
                iterators_dict.pop(resource_type, None)

    def sort(self, data):
        """Сортируем в соответствии со списком полей в params.types."""
        order = self.params['types']
        return sorted(data, key=lambda item: order.index(item['_type']))

    # сериализация
    def serialize(self, data):
        return simplejson.dumps(data, cls=CustomJSONEncoder)

    def build_jsonp_response(self, serialized_data, status_code):
        callback = self.request.GET.get('callback', 'callback')
        data = '%s(%s)' % (callback, serialized_data)
        return HttpResponse(
            content=data,
            content_type='text/javascript',
            status=status_code,
        )

    def build_json_response(self, serialized_data, status_code):
        return HttpResponse(
            content=serialized_data,
            content_type='application/json',
            status=status_code,
        )

    # формирование объекта django.http.HttpResponse
    def build_response(self, serialized_data, status_code=200):
        format = self.request.GET.get('format')
        if format not in self.FORMATS:
            format = self.DEFAULT_FORMAT

        builder_method = getattr(self, 'build_%s_response' % format)
        return builder_method(serialized_data, status_code)
