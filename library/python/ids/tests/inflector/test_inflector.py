# coding: utf-8
from __future__ import unicode_literals

import pytest
import six

from six.moves import mock

from ids.registry import registry
from ids.services.inflector.resource import WordResource, TitleResource
from ids.services.inflector.errors import InflectorParseError, InflectorBadFormatStringError, InflectorBadArguments
from ids.exceptions import IDSException

from .helpers import (
    word_data,
    title_data,
    title_data_parsed,
    build_inflector_answer,
    PersonOnlyRequired,
    Person,
)
from ..helpers import FakeResponse


def test_form_wrong():
    resource = WordResource(build_inflector_answer(word_data))
    with pytest.raises(InflectorBadArguments):
        resource.get_case('badcase')


def test_form_get_item():
    resource = WordResource(build_inflector_answer(word_data))
    for form_eng_name, form_name in six.iteritems(resource.CASE_ATTRIBUTES):
        assert resource[form_name] == getattr(resource, form_eng_name)


def test_title_parse_form_wrong():
    broken_data = {'им': 'broken{Иван}broken{Иванов}broken{Иванович}gender{m}'}
    with pytest.raises(InflectorParseError):
        TitleResource(build_inflector_answer(broken_data.items()))


def test_resource_parse_broken_inflector_answer():
    with pytest.raises(InflectorParseError):
        WordResource({'NoForm': []})


def test_resource_parse_normal_answer():
    res = WordResource({'Form': [
        {'Grammar': 'FakeGrammarName',
         'Text': 'FakeValue'},
        {'Grammar': 'FakeGrammarName2',
         'Text': 'FakeValue2'},
    ]})
    expected_value = {'FakeGrammarName': 'FakeValue',
                      'FakeGrammarName2': 'FakeValue2'}
    assert res.data == expected_value


def test_title_parse_form_empty_value():
    data = {'им': 'persn{Иван}famn{Иванов}patrn{}gender{m}'}
    assert '' == TitleResource(build_inflector_answer(data.items())).format('{middle_name}')


def test_title_parse_form():
    data = {'им': 'persn{Иван}famn{Иванов}patrn{Иванович}gender{m}'}
    res = TitleResource(build_inflector_answer(data.items()))
    assert res.data['им'] == dict(title_data_parsed)['им']


def test_format_bad_format():
    resource = TitleResource(build_inflector_answer(title_data))
    with pytest.raises(InflectorBadFormatStringError):
        resource.format('{wrong_placeholder}')


@pytest.mark.parametrize('fmt_string',
                         ["{first_name} {last_name} {middle_name}",
                          "{first_name} {middle_name} {last_name}",
                          "{last_name} {first_name} {middle_name}",
                          "{first_name}",
                          "{first_name} {last_name}",
                          "{first_name} {middle_name}"])
def test_format(fmt_string):
    resource = TitleResource(build_inflector_answer(title_data))
    case = "дат"
    assert resource.format(fmt_string, case=case) == fmt_string.format(**title_data_parsed[case])


def get_repository():
    return registry.get_repository('inflector', 'inflector', user_agent='test')


def test_person_inflections_bad_arg_dict():
    with pytest.raises(InflectorBadArguments):
        get_repository().get_person_inflections(
            {'no_first_name': '1', 'no_last_name': '2'}
        )


def test_person_inflections_bad_arg_object():
    with pytest.raises(InflectorBadArguments):
        get_repository().get_person_inflections(
            object()
        )


def test_inflect_person_wrong_gender():
    repo = get_repository()
    with pytest.raises(InflectorBadArguments):
        repo.get_person_inflections({'first_name': 'Иван', 'last_name': 'Иванов', 'gender': 'u'})


@mock.patch('requests.Session.request', return_value=FakeResponse(502))
def test_have_ids_exception_on_bad_http_code(mocked_request):
    repo = get_repository()
    with pytest.raises(IDSException):
        repo.inflect_string('Word', 'кому')


def test_inflect_string():
    with mock.patch('ids.services.inflector.repositories.inflector.'
                    'InflectorRepository.get_string_inflections') as mocked:
        repo = get_repository()
        word = "Иван Иванович Иванов"
        form = "кому"
        repo.inflect_string(word, form, True)
        mocked.assert_called_with(word, True)


def test_inflect_person_calls_txt():
    with mock.patch('ids.services.inflector.repositories.inflector.'
                    'InflectorRepository.inflect_string') as mocked:
        repo = get_repository()
        person_txt = "Person Like A String"
        case = "кому"
        repo.inflect_person(person_txt, case)
        mocked.assert_called_with(person_txt, case, inflect_like_fio=True)


def test_inflect_person_calls_dict():
    with mock.patch('ids.services.inflector.repositories.inflector.'
                    'InflectorRepository.get_person_inflections') as mocked:
        repo = get_repository()
        person_dict = {'first_name': 'Иван', 'last_name': 'Иванов', 'gender': 'u'}
        case = "кому"
        repo.inflect_person(person_dict, case)
        mocked.assert_called_with(person_dict)


def test_extract_from_dict():
    repo = get_repository()
    first_name, last_name, middle_name, gender = "fname", "lname", "mname", "m"
    assert repo._extract_from_dict(
        {'first_name': first_name,
         'last_name': last_name,
         'middle_name': middle_name,
         'gender': gender
         }
    ) == (first_name, last_name, middle_name, gender)


def test_extract_from_dict_only_required():
    repo = get_repository()
    first_name, last_name, middle_name, gender = "fname", "lname", "", "m"
    assert repo._extract_from_dict(
        {'first_name': first_name,
         'last_name': last_name,
         }
    ) == (first_name, last_name, middle_name, gender)


def test_extract_from_object():
    repo = get_repository()
    first_name, last_name, middle_name, gender = "Name", "Fname", "Middlename", "f"
    assert repo._extract_from_object(Person(first_name, last_name, middle_name, gender)) == (
        first_name, last_name, middle_name, gender
    )


def test_extract_from_object_only_required():
    repo = get_repository()
    first_name, last_name, middle_name, gender = "Name", "Fname", "", "m"
    assert repo._extract_from_object(PersonOnlyRequired(first_name, last_name)) == (
        first_name, last_name, middle_name, gender
    )
