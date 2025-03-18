# -*- coding: utf-8 -*-

import re
import operator

import six

''' field lookup from http://djangosnippets.org/snippets/2796 '''


_field_lookup_map = {
    "exact": operator.eq,
    "iexact": lambda str1, str2: str1.lower() == str2.lower(),
    "contains": operator.contains,
    "icontains": lambda str1, str2: str2.lower() in str1.lower(),
    "startswith": lambda str1, str2: str1.startswith(str2),
    "istartswith": lambda str1, str2: str1.lower().startswith(str2.lower()),
    "endswith": lambda str1, str2: str1.endswith(str2),
    "iendswith": lambda str1, str2: str1.lower().endswith(str2.lower()),
    "in": lambda obj, iterator: obj in iterator,
    "gt": operator.gt,
    "gte": operator.ge,
    "lt": operator.lt,
    "lte": operator.le,
    "range": lambda val, given_range: val >= given_range[0] and val <= given_range[1],
    #"year": lambda date, year: date.year == year,
    #"month": lambda date, month: date.month == month,
    #"day": lambda date, day: date.day == day,
    #"week_day": lambda date, week_day: (date.isoweekday() + 1) % 7 == week_day,
    "isnull": lambda obj, boolean: (obj is None) == boolean,
    "regex": lambda string, pattern: re.match(pattern, string),
}


def _object_lookupable_single(single_lookup, obj, access_method):
    '''
    @param single_lookup: (str, object)
    @param obj: object
    @returns bool

    @example _object_lookupable_single(('real__real__ge', 1), True)

    Проверяет, подходит ли переданный объект под single field lookup
    в стиле django.
    '''

    fields, target_value = single_lookup
    split_fields = fields.split('__')

    last_field = split_fields[-1]

    # Last field is the operator. If it isn't specified, then
    # exact matching is implied.
    if last_field in _field_lookup_map:
        field_operator = _field_lookup_map[last_field]
        split_fields = split_fields[:-1]
    else:
        field_operator = operator.eq

    # All remaning are related-field lookups. Pop these till we
    # get the field which we actually apply the operator to.

    split_fields.reverse()
    field_value = obj
    while len(split_fields) > 0:
        field_name = split_fields.pop()
        field_value = access_method(field_name)(field_value)

    return field_operator(field_value, target_value)


def object_lookupable(lookup, obj, access_method=operator.itemgetter):
    '''
    @param lookup: dict
    @param obj: object

    @returns bool

    @example object_lookupable({'real__real__ge': 1}, True,
                                    access_method=operator.attrgetter)

    Проверяет, подходит ли переданный объект под field lookup в стиле django.
    '''

    return all(_object_lookupable_single((k, v), obj, access_method)
               for k, v in six.iteritems(lookup))
