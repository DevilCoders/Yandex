# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import types
import pytest
import six

from ids.registry import registry
from ids.services.geobase.repositories.base import Region
from ids.exceptions import BackendError


@pytest.fixture
def parents():
    return registry.get_repository('geobase', 'parents', user_agent='test')


@pytest.fixture
def region():
    return registry.get_repository('geobase', 'region', user_agent='test')


kostroma_id = 7
invalid_id = 287
city_type = 6
country_type = 3
yandex_id = 9999
yandex_ip = '95.108.174.99'


@pytest.mark.integration
def test_region_by_id_ok__get_one(region):
    result = region.get_one({'id': kostroma_id})
    assert isinstance(result, Region)
    assert kostroma_id == result.id
    assert 'Kostroma' == result.en_name
    expected = u'Кострома'
    if not isinstance(result.name, six.text_type):
        expected = expected.encode('utf-8')
    assert expected == result.name
    assert city_type == result.type


@pytest.mark.integration
def test_region_by_id_ok__region_by_id(region):
    result = region.region_by_id(kostroma_id)
    assert isinstance(result, Region)
    assert kostroma_id == result.id
    assert 'Kostroma' == result.en_name
    expected = u'Кострома'
    if not isinstance(result.name, six.text_type):
        expected = expected.encode('utf-8')
    assert expected == result.name
    assert city_type == result.type


@pytest.mark.integration
def test_region_by_id_error(region):
    with pytest.raises(BackendError):
        region.get_one({'id': invalid_id})


@pytest.mark.integration
def test_parents_ok(parents):
    result = parents.get_one({'id': kostroma_id})
    assert isinstance(result, int)

    result = parents.get_all({'id': kostroma_id})
    assert isinstance(result, list)
    assert len(result) > 1
    assert kostroma_id in result
    assert sorted([7, 120957, 10699, 3, 225, 10001, 10000]) == sorted(result)

    result = parents.getiter({'id': kostroma_id})
    assert isinstance(result, types.GeneratorType)
    assert kostroma_id in list(result)


@pytest.mark.integration
def test_parents_error(parents):
    with pytest.raises(BackendError):
        parents.get_one({'id': invalid_id})

    with pytest.raises(BackendError):
        parents.get_all({'id': invalid_id})

    result = parents.getiter({'id': invalid_id})
    with pytest.raises(BackendError):
        list(result)


@pytest.mark.integration
def test_regions_by_type_ok(region):
    result = region.get_one({'type': country_type})
    assert isinstance(result, Region)
    assert country_type == result.type

    result = region.get_all({'type': country_type})
    assert isinstance(result, list)
    assert len(result) > 1
    assert isinstance(result[0], Region)
    assert country_type == result[0].type

    result = region.getiter({'type': country_type})
    assert isinstance(result, types.GeneratorType)
    obj = next(result)
    assert isinstance(obj, Region)
    assert country_type == obj.type


@pytest.mark.integration
def test_regions_by_type_error(region):
    with pytest.raises(BackendError):
        region.get_one({'type': invalid_id})

    with pytest.raises(BackendError):
        region.get_all({'type': invalid_id})

    result = region.getiter({'type': invalid_id})
    with pytest.raises(BackendError):
        list(result)


@pytest.mark.integration
def test_region_by_ip_ok__get_one(region):
    result = region.get_one({'ip': yandex_ip})
    assert isinstance(result, Region)
    assert yandex_id == result.id
    assert 'Yandex' in result.en_name
    assert u'Яндекс' in result.name


@pytest.mark.integration
def test_region_by_ip_ok__region_by_ip(region):
    result = region.region_by_ip(yandex_ip)
    assert isinstance(result, Region)
    assert yandex_id == result.id
    assert 'Yandex' in result.en_name
    assert u'Яндекс' in result.name
