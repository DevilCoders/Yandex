import re
import json
import uuid


def validate_regex(value):
    try:
        return re.compile(value), None
    except re.error as err:
        return False, f'failed to parse regex {value}: {err}'


def validate_int(value):
    try:
        return int(value), None
    except ValueError as err:
        return None, f'failed to parse int: {err}'


def validate_uuid(value):
    try:
        return uuid.UUID(value, version=4), None
    except ValueError as err:
        return None, f'failed to parse uuid: {err}'


class Filter:
    def __init__(
        self,
        params,
        request,
        url=None,
        href=None,
        js_object=None,
    ):
        self.values = {}
        self.queryset = {}
        self.errors = {}
        self.regex = {}
        self.params = params
        self.url = url
        self.href = href
        self.js_object = js_object
        self.__request = request

    def __str__(self):
        return json.dumps(self.as_dict(), indent=4, sort_keys=True, default=str)

    def as_dict(self):
        return {
            'values': self.values,
            'queryset': self.queryset,
            'errors': self.errors,
            'regex': self.regex,
        }

    def parse(self):
        for param in self.params:
            param.parse(self, self.__request)
        return self


class QuerysetParam:
    type = 'input'
    placeholder = 'str'

    def __init__(
        self,
        key,
        name=None,
        strip=True,
        queryset_key=None,
        is_opened=False,
        is_focused=False,
        focus_priority=1,
    ):
        self.key = key
        self.name = name or key.replace('_', ' ').title()
        self.strip = strip
        self.queryset_key = queryset_key
        self.is_opened = is_opened
        self.is_focused = is_focused
        self.focus_priority = focus_priority

    def parse(self, filters, request):
        value = request.GET.get(self.key)

        if value is not None:
            filters.values[self.key] = value
            if self.strip:
                value = value.strip()

            queryset_key = self.queryset_key or self.key
            filters.queryset[queryset_key] = value


class QuerysetListParam:
    type = 'input'
    placeholder = 'comma list'

    def __init__(
        self,
        key,
        name=None,
        strip=True,
        queryset_key=None,
        sep=',',
        validator=None,
        is_opened=False,
        is_focused=False,
        focus_priority=1,
    ):
        self.key = key
        self.name = name or key.replace('_', ' ').title()
        self.strip = strip
        self.queryset_key = queryset_key
        self.sep = sep
        self.validator = None
        self.is_opened = is_opened
        self.is_focused = is_focused
        self.focus_priority = focus_priority

    def parse(self, filters, request):
        values_set = set()
        value = request.GET.get(self.key)

        if value is not None:
            filters.values[self.key] = value
            if self.strip:
                value = value.strip()

            for item in value.split(self.sep):
                if not item:
                    continue
                if self.validator:
                    _, err = self.validator(item)

                    if err:
                        filters.errors[self.key] = err
                        break
                values_set.add(item)

        if values_set:
            queryset_key = self.queryset_key or f'{self.key}__in'
            filters.queryset[queryset_key] = list(values_set)


class QuerysetIntListParam(QuerysetListParam):
    def __init__(self, *args, **kwargs):
        super(QuerysetIntListParam, self).__init__(*args, **kwargs)
        self.validator = validate_int


class QuerysetUuidListParam(QuerysetListParam):
    def __init__(self, *args, **kwargs):
        super(QuerysetUuidListParam, self).__init__(*args, **kwargs)
        self.validator = validate_uuid


class QuerysetRegexParam:
    type = 'input'
    placeholder = 'regex'

    def __init__(
        self,
        key,
        name=None,
        strip=True,
        queryset_key=None,
        is_opened=False,
        is_focused=False,
        focus_priority=1,
    ):
        self.key = key
        self.name = name or key.replace('_', ' ').title()
        self.strip = strip
        self.queryset_key = queryset_key
        self.is_opened = is_opened
        self.is_focused = is_focused
        self.focus_priority = focus_priority

    def parse(self, filters, request):
        value = request.GET.get(self.key)

        if value is not None:
            filters.values[self.key] = value
            if self.strip:
                value = value.strip()

            _, err = validate_regex(value)
            if not err:
                queryset_key = self.queryset_key or f'{self.key}__regex'
                filters.queryset[queryset_key] = value
            else:
                filters.errors[self.key] = err


class RegexParam:
    type = 'input'
    placeholder = 'regex'

    def __init__(
        self,
        key,
        strip=True,
        name=None,
        is_opened=False,
        is_focused=False,
        focus_priority=1,
    ):
        self.key = key
        self.name = name or key.replace('_', ' ').title()
        self.strip = strip
        self.is_opened = is_opened
        self.is_focused = is_focused
        self.focus_priority = focus_priority

    def parse(self, filters, request):
        value = request.GET.get(self.key)

        if value is not None:
            filters.values[self.key] = value
            if self.strip:
                value = value.strip()
            compiled_regex, err = validate_regex(value)
            if not err:
                filters.regex[self.key] = compiled_regex
            else:
                filters.errors[self.key] = err
        return filters


class QuerysetMultipleChoicesParam:
    type = 'checkbox'

    def __init__(
        self,
        key,
        choices,
        name=None,
        strip=True,
        queryset_key=None,
        default_choices=None,
        allow_all=False,
        css_class_prefix=None,
        is_opened=False,
        to=None,
    ):
        self.key = key
        self.name = name or key.replace('_', ' ').title()
        self.choices = choices
        self.strip = strip
        self.queryset_key = queryset_key
        self.default_choices = default_choices
        self.allow_all = allow_all
        self.is_opened = is_opened
        self.css_class_prefix = css_class_prefix
        self.to = to

    def parse(self, filters, request):
        values_set = set()
        value = request.GET.get(self.key)

        if value is not None:
            filters.values[self.key] = value
            if self.strip:
                value = value.strip()

            choices_values = [c[0] for c in self.choices]
            for item in value.split(','):
                if not item:
                    continue
                if self.to:
                    item = self.to(item)
                if item == 'all' and self.allow_all:
                    values_set = set(choices_values)
                    break
                else:
                    if item not in choices_values:
                        continue
                    values_set.add(item)

        if not values_set and self.default_choices:
            values_set = set([c[0] for c in self.default_choices])

        if values_set:
            filters.values[self.key] = values_set
            queryset_key = self.queryset_key or f'{self.key}__in'
            filters.queryset[queryset_key] = list(values_set)
