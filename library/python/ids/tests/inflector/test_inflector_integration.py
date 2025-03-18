# coding: utf-8
from __future__ import unicode_literals

import pytest
import six

from ids.registry import registry

from .helpers import word_data, all_cases, title_data_parsed, Person, PersonOnlyRequired

inflector = registry.get_repository('inflector', 'inflector', user_agent='ids-test')

pytestmark = pytest.mark.integration


def test_word_inflection():
    for case, word_case in six.iteritems(dict(word_data)):
        assert inflector.inflect_string('автомобиль', case) == word_case


def test_word_inflection_getitem():
    res = inflector.get_string_inflections('автомобиль')
    for case, word_case in six.iteritems(dict(word_data)):
        assert res[case] == word_case


def test_word_inflection_attrs():
    res = inflector.get_string_inflections('автомобиль')
    for form_attr, form_name in six.iteritems(res.CASE_ATTRIBUTES):
        assert getattr(res, form_attr) == res[form_name]


def test_person_inflection_as_string():
    parsed_data = dict(title_data_parsed)
    fmt = "{first_name} {last_name}"
    for case in all_cases:
        res = inflector.inflect_person("Иван Иванов", case)
        assert res == fmt.format(**parsed_data[case])


def test_person_inflections_by_dict():
    parsed_data = dict(title_data_parsed)
    res = inflector.get_person_inflections({
        'first_name': 'Иван',
        'last_name': 'Иванов',
        'middle_name': 'Иванович'
    })
    for case in all_cases:
        assert res[case] == parsed_data[case]
    for case_pseudo, case_real in six.iteritems(res.CASES_MAPPING):
        assert res[case_pseudo] == res[case_real]


def test_person_inflections_by_dict_only_required():
    parsed_data = dict(title_data_parsed)
    res = inflector.get_person_inflections({
        'first_name': 'Иван',
        'last_name': 'Иванов',
    })
    for case_pseudo, case_real in six.iteritems(res.CASES_MAPPING):
        assert res[case_pseudo] == res[case_real]


def test_person_inflection_by_object():

    res = inflector.get_person_inflections(
        Person("Иван", "Иванов")
    )
    for case_pseudo, case_real in six.iteritems(res.CASES_MAPPING):
        assert res[case_pseudo] == res[case_real]


def test_person_inflection_by_object_only_required():

    res = inflector.get_person_inflections(
        PersonOnlyRequired("Иван", "Иванов")
    )
    for case_pseudo, case_real in six.iteritems(res.CASES_MAPPING):
        assert res[case_pseudo] == res[case_real]


def test_person_inflection_format():
    res = inflector.get_person_inflections({
        'first_name': 'Иван',
        'last_name': 'Иванов',
        'middle_name': 'Иванович'
    })
    parsed_data = dict(title_data_parsed)
    fmt = "{first_name} {last_name} {middle_name}"
    for case in all_cases:
        assert res.format(fmt, case=case) == fmt.format(**parsed_data[case])

