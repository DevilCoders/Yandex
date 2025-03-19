"""
Tests for property-related methods of HadoopPillar
"""
from dbaas_internal_api.modules.hadoop.pillar import HadoopPillar


# old cluster is cluster that was created before we started to write
# default_properties and user_properties to pillar on cluster creation
def build_pillar(properties: dict, old_cluster: bool):
    data = {'unmanaged': {'properties': properties}}
    if not old_cluster:
        data['auxiliary'] = {
            'default_properties': properties,
            'user_properties': {},
        }
    return HadoopPillar({'data': data})


def test_old_cluster():
    pillar = build_pillar({}, old_cluster=True)
    assert pillar.properties == {}
    assert pillar.user_properties == {}

    new_properties = {'yarn:prop1': 'val1', 'hdfs:prop2': 'val2'}
    pillar.user_properties = new_properties
    assert pillar.properties == new_properties
    assert pillar.user_properties == new_properties

    new_properties = {'yarn:prop1': 'val3', 'hdfs:prop4': 'val4'}
    pillar.user_properties = new_properties
    assert pillar.properties == new_properties
    assert pillar.user_properties == new_properties

    new_properties = {}
    pillar.user_properties = new_properties
    assert pillar.properties == new_properties
    assert pillar.user_properties == new_properties


def test_new_cluster():
    default_properties = {'yarn': {'prop1': 'val1'}, 'hdfs': {'prop2': 'val2'}}
    pillar = build_pillar(default_properties, old_cluster=False)
    assert pillar.properties == {'yarn:prop1': 'val1', 'hdfs:prop2': 'val2'}
    assert pillar.user_properties == {}

    user_properties = {'yarn:prop3': 'val3'}
    pillar.user_properties = user_properties
    assert pillar.properties == {'yarn:prop1': 'val1', 'hdfs:prop2': 'val2', 'yarn:prop3': 'val3'}
    assert pillar.user_properties == user_properties

    user_properties = {'yarn:prop1': 'val3'}
    pillar.user_properties = user_properties
    assert pillar.properties == {'yarn:prop1': 'val3', 'hdfs:prop2': 'val2'}
    assert pillar.user_properties == user_properties

    user_properties = {'yarn:prop1': 'val4'}
    pillar.user_properties = user_properties
    assert pillar.properties == {'yarn:prop1': 'val4', 'hdfs:prop2': 'val2'}
    assert pillar.user_properties == user_properties

    pillar.user_properties = {}
    assert pillar.properties == {'yarn:prop1': 'val1', 'hdfs:prop2': 'val2'}
    assert pillar.user_properties == {}
