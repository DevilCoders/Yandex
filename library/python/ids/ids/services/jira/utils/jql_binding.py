# -*- coding: utf-8 -*-
import six

from ids.exceptions import WrongValueError


_jira_operator_map = {
    'exact': '=',
    'contains': '~',
    'in': 'in',
    'is': 'is',
    'gt': '>',
    'gte': '>=',
    'lt': '<',
    'lte': '<=',

    'nexact': '!=',
    'ncontains': '!~',
    'nin': 'not in',
    'nis': 'is not',
}


def lookup2jql(lookup):
    '''
    @param lookup: dict
    @returns: str

    Преобразует поисковый запрос lookup в jira query language.

    Особые значения:
        '' - преобразуется в empty ('empty' - просто строка)
        None - преобразуется в null ('null' - просто строка)
        Не пустой list и tuple - преобразуются в jql-список
        Пустой list и tuple - таких значений быть не может
        Все значения окружаются кавычками.
    '''

    def encapsulate_value(value):
        encapsulate = True
        if isinstance(value, six.string_types):
            if not value:
                value = u'empty'
                encapsulate = False
        elif value is None:
            value = u'null'
            encapsulate = False

        if encapsulate:
            value = u'"{0}"'.format(value)

        return value

    query_parts = []
    for key in lookup:
        parts = key.split('__')

        operator = parts[-1]
        if operator not in _jira_operator_map:
            operator = 'exact'
            field = '.'.join(parts)
        else:
            field = '.'.join(parts[:-1])

        if not (isinstance(key, six.string_types) and key):
            raise WrongValueError('key must be a string and must not be empty')

        value = lookup[key]
        if isinstance(value, (list, tuple)):
            if not value:
                raise WrongValueError('list should not be empty! probably you want "is empty"')
            # transtale to string and encapsulate with qoutes
            value = map(encapsulate_value, value)
            value = u'({0})'.format(u', '.join(value))
        else:
            value = encapsulate_value(value)

        query_parts.append(u'{0} {1} {2}'.format(
                                            field,
                                            _jira_operator_map[operator],
                                            value,
                                        )
        )

    return u' AND '.join(query_parts)
