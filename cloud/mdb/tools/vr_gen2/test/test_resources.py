import library.python.resource
import pytest
from cloud.mdb.tools.vr_gen2.internal.data_sources import (
    DiskTypeIdSource,
    GeoIdSource,
    FlavorSource,
)
from hamcrest import assert_that, is_not, has_items, has_entries


def get_resource(name) -> str:
    result = library.python.resource.find(f'resfs/file/resources/datasource/{name}')
    assert result is not None, 'missing in ya.make?'
    return result


def test_disk_type():
    source = DiskTypeIdSource(get_resource('disk_type.yaml'))
    assert source.get_by_ext_id('gp2') == 1
    with pytest.raises(KeyError):
        source.get_by_ext_id('some-other-type')


def test_geo():
    source = GeoIdSource(get_resource('geo.yaml'))
    assert source.get_without_names(['aws-london']) == [1]


def test_flavor_invisible():
    source = FlavorSource(get_resource('flavor.yaml'))
    assert_that(source.flavors, is_not(has_items(has_entries({'name': 'f.invisible'}))))


def test_flavor_by_name():
    source = FlavorSource(get_resource('flavor.yaml'))
    result = source.filter_ids_by_name('s1.nano')
    assert len(result) == 1
    assert result[0] == '32165f32-34c9-4e2d-b150-d07699b73cf4'


def test_flavor_by_name_unknown():
    source = FlavorSource(get_resource('flavor.yaml'))
    result = source.filter_ids_by_name('f.invisible')
    assert len(result) == 0


def test_flavor_by_type():
    source = FlavorSource(get_resource('flavor.yaml'))
    result = source.filter_ids_by_type('aws-standard')
    assert len(result) == 1
    assert result[0] == '32165f32-34c9-4e2d-b150-d07699b73cf4'


def test_flavor_by_type_unknown():
    source = FlavorSource(get_resource('flavor.yaml'))
    result = source.filter_ids_by_type('aws-nonstandard')
    assert len(result) == 0
