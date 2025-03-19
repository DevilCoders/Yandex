import library.python.resource
import pytest
from cloud.mdb.tools.vr_gen2.internal.schema import load_resource_definitions
from marshmallow import exceptions


def get_resource(name) -> str:
    result = library.python.resource.find(f'resfs/file/{name}')
    assert result is not None, "missing in ya.make?"
    return result


def test_parses_clickhouse():
    resources = get_resource('resources/clickhouse/defs.yaml')
    defs = load_resource_definitions(resources)
    ch_roles = defs.cluster_types.clickhouse_cluster
    assert len(ch_roles.zk.gp2) == 1
    resource = ch_roles.zk.gp2[0]
    assert resource.feature_flags == ["FFLAG", "FFLAG2"]
    assert resource.flavor_types[0].name == "s1.nano"
    assert resource.host_count[0].max == 3
    assert resource.host_count[0].min == 1
    assert resource.disk_sizes[0].int8range.start == 10
    assert resource.disk_sizes[0].int8range.end == 20
    assert resource.disk_sizes[1].custom_range.min == "93GB"
    assert resource.disk_sizes[1].custom_range.step == "93GB"
    assert resource.disk_sizes[1].custom_range.upto == "8TB"
    assert resource.disk_sizes[2].custom_sizes == ["1GB", "2GB"]
    assert resource.excluded_geo == ["geo1"]


def test_parses_kafka():
    resources = get_resource('resources/kafka/defs.yaml')
    defs = load_resource_definitions(resources)
    kf_roles = defs.cluster_types.kafka_cluster
    assert len(kf_roles.zk.gp2) == 1
    resource = kf_roles.zk.gp2[0]
    assert resource.feature_flags == ["FFLAG", "FFLAG2"]
    assert resource.flavor_types[0].name == "s1.nano"
    assert resource.host_count[0].max == 3
    assert resource.host_count[0].min == 1
    assert resource.disk_sizes[0].int8range.start == 10
    assert resource.disk_sizes[0].int8range.end == 20
    assert resource.disk_sizes[1].custom_range.min == "93GB"
    assert resource.disk_sizes[1].custom_range.step == "93GB"
    assert resource.disk_sizes[1].custom_range.upto == "8TB"
    assert resource.excluded_geo == []


def test_fails_unknown_property():
    resources = get_resource('resources/unknown_property/defs.yaml')
    with pytest.raises(exceptions.ValidationError) as e_info:
        load_resource_definitions(resources)
        assert '''{'cluster_types': {'unknown_cluster': ['Unknown field.']}}''' in str(e_info)


def test_parses_empty_file():
    resources = get_resource('resources/empty/defs.yaml')
    load_resource_definitions(resources)
